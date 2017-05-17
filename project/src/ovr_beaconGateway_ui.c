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
#include "ovr_beaconGateway_ui.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_mqtt_client.h>
#include <cxa_mqtt_connectionManager.h>
#include <cxa_network_wifiManager.h>
#include <ovr_beaconGateway.h>


// ******** local macro definitions ********
#define WIFI_RGBCOLOR_UNKNOWN				0,   0,   0
#define WIFI_RGBCOLOR_ERROR					255, 0,   0
#define WIFI_RGBCOLOR_PROVISION				0,   255, 0
#define WIFI_RGBCOLOR_CONNECTING			0,   0,   255
#define WIFI_RGBCOLOR_CONNECTED				0,   0,   255
#define WIFI_RGBCOLOR_CONNECTED_ACT			0,   0,   0

#define BTLE_RGBCOLOR_IDLE  				0,   0,   255
#define BTLE_RGBCOLOR_ERROR					255, 0,   0
#define BTLE_RGBCOLOR_CONNECTED_ACT  		0,   0,   0

#define BLINKRATE_ERROR						100,  100
#define BLINKRATE_PROVISION					900,  100
#define BLINKRATE_PROVISION_LPM				250,  250
#define BLINKRATE_CONNECTING				250,  250

#define FLASHPERIOD_ACT_MS					100


// ******** local type definitions ********


// ******** local function prototypes ********
static void updateNetLed(ovr_beaconGateway_ui_t *const bguiIn);

static void wifiCb_onProvisioning(void* userVarIn);
static void wifiCb_onAssociating(const char *const ssidIn, void* userVarIn);
static void wifiCb_onAssociationFailed(const char *const ssidIn, void* userVarIn);

static void lpmCb_onEnter_provision(void* userVarIn);
static void lpmCb_onLeave_provision(void* userVarIn);
static void lpmCb_onSelected_provision(void* userVarIn);

static void mqttCb_onConnect(cxa_mqtt_client_t *const clientIn, void* userVarIn);
static void mqttCb_onConnectFailed(cxa_mqtt_client_t *const clientIn, cxa_mqtt_client_connectFailureReason_t reasonIn, void* userVarIn);
static void mqttCb_onDisconnect(cxa_mqtt_client_t *const clientIn, void* userVarIn);
static void mqttCb_onPingRespRx(cxa_mqtt_client_t *const clientIn, void* userVarIn);

static void btleCb_onReady(cxa_btle_client_t *const btlecIn, void* userVarIn);
static void btleCb_onFailedInit(cxa_btle_client_t *const btlecIn, bool willAutoRetryIn, void* userVarIn);

static void beaconManagerCb_onBeaconUpdate(ovr_beaconProxy_t *const beaconProxyIn, void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void ovr_beaconGateway_ui_init(ovr_beaconGateway_ui_t *const bguiIn,
							   cxa_btle_client_t *const btleClientIn, ovr_beaconManager_t *const bmIn,
							   cxa_rgbLed_t *const led_btleActIn, cxa_rgbLed_t *const led_netActIn,
							   cxa_gpio_t *const gpio_swProvIn)
{
	cxa_assert(bguiIn);
	cxa_assert(btleClientIn);
	cxa_assert(bmIn);
	cxa_assert(gpio_swProvIn);

	// save our references
	bguiIn->led_btleAct = led_btleActIn;
	bguiIn->led_netAct = led_netActIn;

	// set our initial state
	bguiIn->networkError = false;
	cxa_rgbLed_setRgb(bguiIn->led_btleAct, WIFI_RGBCOLOR_UNKNOWN);
	cxa_rgbLed_setRgb(bguiIn->led_netAct, WIFI_RGBCOLOR_UNKNOWN);

	// setup our long press manager
	cxa_gpio_longPressManager_init(&bguiIn->lpm_swProv, gpio_swProvIn, OVR_GW_THREADID_UI);
	cxa_gpio_longPressManager_addSegment(&bguiIn->lpm_swProv, 5000, UINT16_MAX, lpmCb_onEnter_provision, lpmCb_onLeave_provision, lpmCb_onSelected_provision, (void*)bguiIn);

	// register for wifi callbacks
	cxa_network_wifiManager_addListener(NULL, wifiCb_onProvisioning, wifiCb_onAssociating,
										NULL, NULL, wifiCb_onAssociationFailed, NULL, (void*)bguiIn);

	// register for mqtt client callbacks
	cxa_mqtt_client_t *mqttC = cxa_mqtt_connManager_getMqttClient();
	if( mqttC != NULL )
	{
		cxa_mqtt_client_addListener(mqttC,
									mqttCb_onConnect, mqttCb_onConnectFailed, mqttCb_onDisconnect, mqttCb_onPingRespRx,
									(void*)bguiIn);
	}

	// register for btle callbacks
	cxa_btle_client_addListener(btleClientIn, btleCb_onReady, btleCb_onFailedInit, (void*)bguiIn);

	// register for beacon activity callbacks
	ovr_beaconManager_addListener(bmIn, NULL, beaconManagerCb_onBeaconUpdate, NULL, (void*)bguiIn);
}


// ******** local function implementations ********
static void updateNetLed(ovr_beaconGateway_ui_t *const bguiIn)
{
	cxa_assert(bguiIn);
	if( bguiIn->led_netAct == NULL ) return;

	// don't do anything if our button is pressed (user interacting)
	if( cxa_gpio_longPressManager_isPressed(&bguiIn->lpm_swProv) ) return;

	// if we made it here, our button is not pressed...
	if( cxa_network_wifiManager_getState() == CXA_NETWORK_WIFISTATE_PROVISIONING )
	{
		cxa_rgbLed_blink(bguiIn->led_netAct, WIFI_RGBCOLOR_PROVISION, BLINKRATE_PROVISION);
	}
	else if( cxa_mqtt_client_isConnected(cxa_mqtt_connManager_getMqttClient()) )
	{
		cxa_rgbLed_setRgb(bguiIn->led_netAct, WIFI_RGBCOLOR_CONNECTED);
	}
	else if( bguiIn->networkError )
	{
		cxa_rgbLed_blink(bguiIn->led_netAct, WIFI_RGBCOLOR_ERROR, BLINKRATE_ERROR);
	}
	else
	{
		cxa_rgbLed_blink(bguiIn->led_netAct, WIFI_RGBCOLOR_CONNECTING, BLINKRATE_CONNECTING);
	}
}


static void wifiCb_onProvisioning(void* userVarIn)
{
	ovr_beaconGateway_ui_t* bguiIn = (ovr_beaconGateway_ui_t*)userVarIn;
	cxa_assert(bguiIn);

	updateNetLed(bguiIn);
}


static void wifiCb_onAssociating(const char *const ssidIn, void* userVarIn)
{
	ovr_beaconGateway_ui_t* bguiIn = (ovr_beaconGateway_ui_t*)userVarIn;
	cxa_assert(bguiIn);

	updateNetLed(bguiIn);
}


static void wifiCb_onAssociationFailed(const char *const ssidIn, void* userVarIn)
{
	ovr_beaconGateway_ui_t* bguiIn = (ovr_beaconGateway_ui_t*)userVarIn;
	cxa_assert(bguiIn);

	bguiIn->networkError = true;
	updateNetLed(bguiIn);
}


static void lpmCb_onEnter_provision(void* userVarIn)
{
	ovr_beaconGateway_ui_t* bguiIn = (ovr_beaconGateway_ui_t*)userVarIn;
	cxa_assert(bguiIn);

	cxa_rgbLed_blink(bguiIn->led_netAct, WIFI_RGBCOLOR_PROVISION, BLINKRATE_PROVISION_LPM);
}


static void lpmCb_onLeave_provision(void* userVarIn)
{
	ovr_beaconGateway_ui_t* bguiIn = (ovr_beaconGateway_ui_t*)userVarIn;
	cxa_assert(bguiIn);

	updateNetLed(bguiIn);
}


static void lpmCb_onSelected_provision(void* userVarIn)
{
	ovr_beaconGateway_ui_t* bguiIn = (ovr_beaconGateway_ui_t*)userVarIn;
	cxa_assert(bguiIn);

	cxa_network_wifiManager_enterProvision();
}


static void mqttCb_onConnect(cxa_mqtt_client_t *const clientIn, void* userVarIn)
{
	ovr_beaconGateway_ui_t* bguiIn = (ovr_beaconGateway_ui_t*)userVarIn;
	cxa_assert(bguiIn);

	bguiIn->networkError = false;
	updateNetLed(bguiIn);
}


static void mqttCb_onConnectFailed(cxa_mqtt_client_t *const clientIn, cxa_mqtt_client_connectFailureReason_t reasonIn, void* userVarIn)
{
	ovr_beaconGateway_ui_t* bguiIn = (ovr_beaconGateway_ui_t*)userVarIn;
	cxa_assert(bguiIn);

	bguiIn->networkError = true;
	updateNetLed(bguiIn);
}


static void mqttCb_onDisconnect(cxa_mqtt_client_t *const clientIn, void* userVarIn)
{
	ovr_beaconGateway_ui_t* bguiIn = (ovr_beaconGateway_ui_t*)userVarIn;
	cxa_assert(bguiIn);

	updateNetLed(bguiIn);
}


static void mqttCb_onPingRespRx(cxa_mqtt_client_t *const clientIn, void* userVarIn)
{
	ovr_beaconGateway_ui_t* bguiIn = (ovr_beaconGateway_ui_t*)userVarIn;
	cxa_assert(bguiIn);

	if( bguiIn->led_netAct ) cxa_rgbLed_flashOnce(bguiIn->led_netAct, WIFI_RGBCOLOR_CONNECTED_ACT, FLASHPERIOD_ACT_MS);
}


static void btleCb_onReady(cxa_btle_client_t *const btlecIn, void* userVarIn)
{
	ovr_beaconGateway_ui_t* bguiIn = (ovr_beaconGateway_ui_t*)userVarIn;
	cxa_assert(bguiIn);

	if( bguiIn->led_btleAct ) cxa_rgbLed_setRgb(bguiIn->led_btleAct, BTLE_RGBCOLOR_IDLE);
}


static void btleCb_onFailedInit(cxa_btle_client_t *const btlecIn, bool willAutoRetryIn, void* userVarIn)
{
	ovr_beaconGateway_ui_t* bguiIn = (ovr_beaconGateway_ui_t*)userVarIn;
	cxa_assert(bguiIn);

	if( bguiIn->led_btleAct ) cxa_rgbLed_blink(bguiIn->led_btleAct, BTLE_RGBCOLOR_ERROR, BLINKRATE_ERROR);
}


static void beaconManagerCb_onBeaconUpdate(ovr_beaconProxy_t *const beaconProxyIn, void* userVarIn)
{
	ovr_beaconGateway_ui_t* bguiIn = (ovr_beaconGateway_ui_t*)userVarIn;
	cxa_assert(bguiIn);

	if( bguiIn->led_btleAct ) cxa_rgbLed_flashOnce(bguiIn->led_btleAct, BTLE_RGBCOLOR_CONNECTED_ACT, FLASHPERIOD_ACT_MS);
}
