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
#include "ovr_beaconManager_rpcInterface.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_stringUtils.h>

#include <ovr_beaconManager.h>
#include <ovr_beaconProxy.h>
#include <ovr_beaconUpdate.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define UPDATE_MAX_PAYLOAD_BYTES				256


// ******** local type definitions ********


// ******** local function prototypes ********
static void beaconCb_onBeaconUpdated(ovr_beaconProxy_t *const beaconProxyIn, void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void ovr_beaconManager_rpcInterface_init(ovr_beaconManager_rpcInterface_t *const bmriIn, ovr_beaconManager_t *const bmIn, cxa_mqtt_rpc_node_t *const rpcNodeIn)
{
	cxa_assert(bmriIn);
	cxa_assert(bmIn);
	cxa_assert(rpcNodeIn);

	// save our references
	bmriIn->bm = bmIn;
	bmriIn->rpcNode = rpcNodeIn;

	// register for beacon events
	ovr_beaconManager_addListener(bmriIn->bm, NULL, beaconCb_onBeaconUpdated, NULL, (void*)bmriIn);
}


// ******** local function implementations ********
static void beaconCb_onBeaconUpdated(ovr_beaconProxy_t *const beaconProxyIn, void* userVarIn)
{
	ovr_beaconManager_rpcInterface_t* bmriIn = (ovr_beaconManager_rpcInterface_t*)userVarIn;
	cxa_assert(bmriIn);

	// send an update notification...unfortunately it needs to be JSON for AWS IoT :'(
	ovr_beaconUpdate_t* lastUpdate = ovr_beaconProxy_getLastUpdate(beaconProxyIn);
	if( lastUpdate == NULL ) return;

	// get our individual strings together
	cxa_uuid128_t updateUuid;
	cxa_uuid128_string_t updateUuid_str;
	cxa_uuid128_initRandom(&updateUuid);
	cxa_uuid128_toString(&updateUuid, &updateUuid_str);

	cxa_uuid128_string_t uuid_str;
	cxa_uuid128_toString(ovr_beaconProxy_getUuid128(beaconProxyIn), &uuid_str);

	char temp_str[4];
	snprintf(temp_str, sizeof(temp_str), "%d", ovr_beaconUpdate_getTemp_c(lastUpdate));
	temp_str[sizeof(temp_str)-1] = 0;

	char rssi_str[5];
	snprintf(rssi_str, sizeof(rssi_str), "%d", ovr_beaconUpdate_getRssi(lastUpdate));
	rssi_str[sizeof(rssi_str)-1] = 0;

	char batt_str[4];
	snprintf(batt_str, sizeof(batt_str), "%d", ovr_beaconUpdate_getBattery_pcnt100(lastUpdate));
	batt_str[sizeof(batt_str)-1] = 0;

	// combine into one payload string
	char notiPayload[UPDATE_MAX_PAYLOAD_BYTES] = "";
	if( !cxa_stringUtils_concat(notiPayload, "{\"updateUuid\":\"", sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, updateUuid_str.str, sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, "\",\"beaconUuid\":\"", sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, uuid_str.str, sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, "\",\"temp_c\":", sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, temp_str, sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, ",\"rssi\":", sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, rssi_str, sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, ",\"batt_pcnt\":", sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, batt_str, sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, "}", sizeof(notiPayload)) ) return;

	cxa_mqtt_rpc_node_publishNotification(bmriIn->rpcNode, "onBeaconUpdate", CXA_MQTT_QOS_ATMOST_ONCE, notiPayload, strlen(notiPayload));
}
