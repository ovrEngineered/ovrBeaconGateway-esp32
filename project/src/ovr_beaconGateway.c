/**
 * @copyright 2017 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#include "ovr_beaconGateway.h"


// ******** includes ********
#include <math.h>

#include <cxa_assert.h>
#include <cxa_console.h>
#include <cxa_network_wifiManager.h>
#include <cxa_runLoop.h>
#include <cxa_uniqueId.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define SENSOR_READ_PERIOD_MS		60000
//#define SENSOR_READ_PERIOD_MS		10000


// ******** local type definitions ********


// ******** local function prototypes ********
static void cb_onRunLoopUpdate(void* userVarIn);

static void consoleCb_getUuid(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);

static void tempCb_onUpdated(cxa_tempSensor_t *const tmpSnsIn, bool wasSuccessfulIn, float newTemp_degCIn, void* userVarIn);
static void lightCb_onUpdated(cxa_lightSensor_t *const lightSnsIn, bool wasSuccessfulIn, uint8_t newLight_255In, void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void ovr_beaconGateway_init(ovr_beaconGateway_t *const bgIn,
							cxa_btle_client_t *const btleClientIn,
							cxa_gpio_t *const gpio_swProvisionIn,
							cxa_gpio_t *const gpio_variant_internalHighPowerIn,
							cxa_gpio_t *const gpio_variant_externalIn,
							cxa_rgbLed_t *const led_btleActIn,
							cxa_rgbLed_t *const led_netActIn,
							cxa_lightSensor_t *const lightSensorIn,
							cxa_tempSensor_t *const tempSensorIn,
							cxa_mqtt_rpc_node_t *const rpcNodeIn)
{
	cxa_assert(bgIn);
	cxa_assert(btleClientIn);
	cxa_assert(gpio_variant_internalHighPowerIn);
	cxa_assert(gpio_variant_externalIn);

	bgIn->btleClient = btleClientIn;
	cxa_logger_init(&bgIn->logger, "beaconGateway");

	bgIn->lightSensor = lightSensorIn;
	bgIn->tempSensor = tempSensorIn;
	cxa_timeDiff_init(&bgIn->td_readSensors);

	// determine our variant (gpios should be inverted)
	if( cxa_gpio_getValue(gpio_variant_internalHighPowerIn) && !cxa_gpio_getValue(gpio_variant_externalIn) )
	{
		bgIn->variant = OVR_BEACONGW_VARIANT_INTERNAL_HIGHPOWER;
	}
	else if( !cxa_gpio_getValue(gpio_variant_internalHighPowerIn) && cxa_gpio_getValue(gpio_variant_externalIn) )
	{
		bgIn->variant = OVR_BEACONGW_VARIANT_EXTERNAL;
	}
	else if( !cxa_gpio_getValue(gpio_variant_internalHighPowerIn) && !cxa_gpio_getValue(gpio_variant_externalIn) )
	{
		bgIn->variant = OVR_BEACONGW_VARIANT_INTERNAL;
	}
	else
	{
		bgIn->variant = OVR_BEACONGW_VARIANT_UNKNOWN;
	}
	cxa_logger_info(&bgIn->logger, "variant: %d", bgIn->variant);


	// setup our beacon manager
	ovr_beaconManager_init(&bgIn->beaconManager, btleClientIn, rpcNodeIn);

	// setup our rpc interface
	if( rpcNodeIn != NULL ) ovr_beaconGateway_rpcInterface_init(&bgIn->bgri, bgIn, rpcNodeIn);

	// setup our UI
	ovr_beaconGateway_ui_init(&bgIn->bgui, btleClientIn, &bgIn->beaconManager, led_btleActIn, led_netActIn, gpio_swProvisionIn);

	// register our console method
	cxa_console_addCommand("gw_getUuid", "returns gateway's UUID", NULL, 0, consoleCb_getUuid, (void*)bgIn);

	// schedule for repeated execution
	cxa_runLoop_addEntry(OVR_GW_THREADID_BLUETOOTH, cb_onRunLoopUpdate, (void*)bgIn);
}


bool ovr_beaconGateway_isBeaconRadioReady(ovr_beaconGateway_t *const bgIn)
{
	cxa_assert(bgIn);
	return ovr_beaconManager_isRadioReady(&bgIn->beaconManager);
}


float ovr_beaconGateway_getLastTemp_degC(ovr_beaconGateway_t *const bgIn)
{
	cxa_assert(bgIn);

	return (bgIn->tempSensor != NULL) ? cxa_tempSensor_getLastValue_degC(bgIn->tempSensor) : NAN;
}


uint8_t ovr_beaconGateway_getLastLight_255(ovr_beaconGateway_t *const bgIn)
{
	cxa_assert(bgIn);

	return (bgIn->lightSensor != NULL) ? cxa_lightSensor_getLastValue_255(bgIn->lightSensor) : 0;
}


ovr_beaconGateway_variant_t ovr_beaconGateway_getVariant(ovr_beaconGateway_t *const bgIn)
{
	cxa_assert(bgIn);

	return bgIn->variant;
}


void ovr_beaconGateway_onAssert(ovr_beaconGateway_t *const bgIn)
{
	ovr_beaconGateway_ui_onAssert(&bgIn->bgui);
}


// ******** local function implementations ********
static void cb_onRunLoopUpdate(void* userVarIn)
{
	ovr_beaconGateway_t* bgIn = (ovr_beaconGateway_t*)userVarIn;
	cxa_assert(bgIn);

	if( cxa_btle_client_isReady(bgIn->btleClient) && cxa_timeDiff_isElapsed_recurring_ms(&bgIn->td_readSensors, SENSOR_READ_PERIOD_MS) )
	{
		if( bgIn->tempSensor != NULL )
		{
			cxa_tempSensor_getValue_withCallback(bgIn->tempSensor, tempCb_onUpdated, (void*)bgIn);
		}
		else if( bgIn->lightSensor != NULL )
		{
			cxa_lightSensor_getValue_withCallback(bgIn->lightSensor, lightCb_onUpdated, (void*)bgIn);
		}
	}
}


static void consoleCb_getUuid(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	ovr_beaconGateway_t* bgIn = (ovr_beaconGateway_t*)userVarIn;
	cxa_assert(bgIn);

	cxa_ioStream_writeLine(ioStreamIn, cxa_uniqueId_getHexString());
}


static void tempCb_onUpdated(cxa_tempSensor_t *const tmpSnsIn, bool wasSuccessfulIn, float newTemp_degCIn, void* userVarIn)
{
	ovr_beaconGateway_t* bgIn = (ovr_beaconGateway_t*)userVarIn;
	cxa_assert(bgIn);

	if( !wasSuccessfulIn ) cxa_logger_warn(&bgIn->logger, "failed to read gwTemp");

	cxa_lightSensor_getValue_withCallback(bgIn->lightSensor, lightCb_onUpdated, (void*)bgIn);
}


static void lightCb_onUpdated(cxa_lightSensor_t *const lightSnsIn, bool wasSuccessfulIn, uint8_t newLight_255In, void* userVarIn)
{
	ovr_beaconGateway_t* bgIn = (ovr_beaconGateway_t*)userVarIn;
	cxa_assert(bgIn);

	if( !wasSuccessfulIn ) cxa_logger_warn(&bgIn->logger, "failed to read gwLux");
}
