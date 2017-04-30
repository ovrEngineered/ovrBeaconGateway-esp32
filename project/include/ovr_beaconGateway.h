/**
 * @file
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
#ifndef OVR_BEACONGATEWAY_H_
#define OVR_BEACONGATEWAY_H_


// ******** includes ********
#include <cxa_led.h>
#include <cxa_lightSensor.h>
#include <cxa_logger_header.h>
#include <cxa_rgbLed.h>
#include <cxa_tempSensor.h>
#include <cxa_timeDiff.h>

#include <ovr_beaconGateway_ui.h>
#include <ovr_beaconGateway_rpcInterface.h>
#include <ovr_beaconManager.h>


// ******** global macro definitions ********
#define OVR_GW_THREADID_NETWORK				1
#define OVR_GW_THREADID_UI					2
#define OVR_GW_THREADID_BLUETOOTH			3


// ******** global type definitions *********
/**
 * @public
 * Forward declaration
 */
typedef struct ovr_beaconGateway ovr_beaconGateway_t;


/**
 * @private
 */
struct ovr_beaconGateway
{
	cxa_btle_client_t* btleClient;
	ovr_beaconManager_t beaconManager;

	ovr_beaconGateway_rpcInterface_t bgri;

	ovr_beaconGateway_ui_t bgui;

	cxa_timeDiff_t td_readSensors;
	cxa_lightSensor_t* lightSensor;
	cxa_tempSensor_t* tempSensor;

	cxa_logger_t logger;
};


// ******** global function prototypes ********
/**
 * @public
 */
void ovr_beaconGateway_init(ovr_beaconGateway_t *const bgIn,
							cxa_btle_client_t *const btleClientIn,
							cxa_rgbLed_t *const led_btleActIn,
							cxa_rgbLed_t *const led_netActIn,
							cxa_lightSensor_t *const lightSensorIn,
							cxa_tempSensor_t *const tempSensorIn,
							cxa_mqtt_rpc_node_t *const rpcNodeIn);


bool ovr_beaconGateway_isBeaconRadioReady(ovr_beaconGateway_t *const bgIn);
float ovr_beaconGateway_getLastTemp_degC(ovr_beaconGateway_t *const bgIn);
uint8_t ovr_beaconGateway_getLastLight_255(ovr_beaconGateway_t *const bgIn);

#endif
