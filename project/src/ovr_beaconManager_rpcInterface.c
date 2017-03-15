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
#include <cxa_runLoop.h>
#include <cxa_sntpClient.h>
#include <cxa_stringUtils.h>
#include <cxa_uniqueId.h>
#include <cxa_uuid128.h>

#include <ovr_beaconManager.h>
#include <ovr_beaconProxy.h>
#include <ovr_beaconUpdate.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define UPDATE_MAX_PAYLOAD_BYTES				256
#define UPDATE_PERIOD_MS						60000


// ******** local type definitions ********


// ******** local function prototypes ********


static void cb_onRunLoopUpdate(void* userVarIn);
static void beaconCb_onBeaconFound(ovr_beaconProxy_t *const beaconProxyIn, void* userVarIn);
static void beaconCb_onBeaconLost(ovr_beaconProxy_t *const beaconProxyIn, void* userVarIn);


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

	cxa_timeDiff_init(&bmriIn->td_sendUpdate);

	// register for beacon events
	ovr_beaconManager_addListener(bmriIn->bm, beaconCb_onBeaconFound, NULL, beaconCb_onBeaconLost, (void*)bmriIn);

	// register for runloop updates
	cxa_runLoop_addEntry(cb_onRunLoopUpdate, (void*)bmriIn);
}


// ******** local function implementations ********
static void cb_onRunLoopUpdate(void* userVarIn)
{
	ovr_beaconManager_rpcInterface_t* bmriIn = (ovr_beaconManager_rpcInterface_t*)userVarIn;
	cxa_assert(bmriIn);

	if( cxa_sntpClient_isClockSet() && cxa_timeDiff_isElapsed_recurring_ms(&bmriIn->td_sendUpdate, UPDATE_PERIOD_MS) )
	{
		// iterate over our beacons and send last-known values
		cxa_array_iterate(ovr_beaconManager_getKnownBeacons(bmriIn->bm), currBeacon, ovr_beaconProxy_t)
		{
			if( currBeacon == NULL ) continue;

			ovr_beaconUpdate_t* lastUpdate = ovr_beaconProxy_getLastUpdate(currBeacon);
			if( lastUpdate == NULL ) continue;

			// get our individual strings together
			char* gatewayUniqueId = cxa_uniqueId_getHexString();

			char timestamp_str[11];
			snprintf(timestamp_str, sizeof(timestamp_str), "%d", cxa_sntpClient_getUnixTimeStamp());
			timestamp_str[sizeof(timestamp_str)-1] = 0;

			cxa_eui48_string_t uuid_str;
			cxa_eui48_toString(ovr_beaconProxy_getEui48(currBeacon), &uuid_str);

			char isCharging_str[2];
			snprintf(isCharging_str, sizeof(isCharging_str), "%d", ovr_beaconUpdate_getIsCharging(lastUpdate));
			isCharging_str[sizeof(isCharging_str)-1] = 0;

			char isActive_str[2];
			snprintf(isActive_str, sizeof(isActive_str), "%d", ovr_beaconProxy_checkAndResetPendingActivity(currBeacon));
			isActive_str[sizeof(isActive_str)-1] = 0;

			char rssi_str[5];
			snprintf(rssi_str, sizeof(rssi_str), "%d", ovr_beaconUpdate_getRssi(lastUpdate));
			rssi_str[sizeof(rssi_str)-1] = 0;

			char temp_str[4];
			snprintf(temp_str, sizeof(temp_str), "%d", ovr_beaconUpdate_getTemp_c(lastUpdate));
			temp_str[sizeof(temp_str)-1] = 0;

			char light_str[4];
			snprintf(light_str, sizeof(light_str), "%d", ovr_beaconUpdate_getLight_255(lastUpdate));
			light_str[sizeof(light_str)-1] = 0;

			char battPcnt_str[4];
			snprintf(battPcnt_str, sizeof(battPcnt_str), "%d", ovr_beaconUpdate_getBattery_pcnt100(lastUpdate));
			battPcnt_str[sizeof(battPcnt_str)-1] = 0;

			char battV_str[6];
			snprintf(battV_str, sizeof(battV_str), "%0.2f", (float)ovr_beaconUpdate_getBattery_mv(lastUpdate)/1000.0);
			battV_str[sizeof(battV_str)-1] = 0;

			// combine into one payload string
			char notiPayload[UPDATE_MAX_PAYLOAD_BYTES] = "";
			if( !cxa_stringUtils_concat(notiPayload, "{\"gatewayId\":\"", sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, gatewayUniqueId, sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, "\",\"timestamp\":", sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, timestamp_str, sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, ",\"beaconId\":\"", sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, uuid_str.str, sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, "\",\"rssi\":", sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, rssi_str, sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, ",\"isActive\":", sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, isActive_str, sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, ",\"isCharging\":", sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, isCharging_str, sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, ",\"temp_c\":", sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, temp_str, sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, ",\"light_255\":", sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, light_str, sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, ",\"batt_pcnt100\":", sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, battPcnt_str, sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, ",\"batt_v\":", sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, battV_str, sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, "}", sizeof(notiPayload)) ) return;

			cxa_mqtt_rpc_node_publishNotification(bmriIn->rpcNode, "onBeaconUpdate", CXA_MQTT_QOS_ATMOST_ONCE, notiPayload, strlen(notiPayload));
		}
	}
}


static void beaconCb_onBeaconFound(ovr_beaconProxy_t *const beaconProxyIn, void* userVarIn)
{
	ovr_beaconManager_rpcInterface_t* bmriIn = (ovr_beaconManager_rpcInterface_t*)userVarIn;
	cxa_assert(bmriIn);

	cxa_assert(beaconProxyIn);

	if( !cxa_sntpClient_isClockSet() ) return;

	ovr_beaconUpdate_t* lastUpdate = ovr_beaconProxy_getLastUpdate(beaconProxyIn);
	if( lastUpdate == NULL ) return;

	// get our individual strings together
	char* gatewayUniqueId = cxa_uniqueId_getHexString();

	char timestamp_str[11];
	snprintf(timestamp_str, sizeof(timestamp_str), "%d", cxa_sntpClient_getUnixTimeStamp());
	timestamp_str[sizeof(timestamp_str)-1] = 0;

	cxa_eui48_string_t uuid_str;
	cxa_eui48_toString(ovr_beaconProxy_getEui48(beaconProxyIn), &uuid_str);

	// combine into one payload string
	char notiPayload[UPDATE_MAX_PAYLOAD_BYTES] = "";
	if( !cxa_stringUtils_concat(notiPayload, "{\"gatewayId\":\"", sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, gatewayUniqueId, sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, "\",\"timestamp\":", sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, timestamp_str, sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, ",\"beaconId\":\"", sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, uuid_str.str, sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, "\"}", sizeof(notiPayload)) ) return;

	cxa_mqtt_rpc_node_publishNotification(bmriIn->rpcNode, "onBeaconFound", CXA_MQTT_QOS_ATMOST_ONCE, notiPayload, strlen(notiPayload));
}


static void beaconCb_onBeaconLost(ovr_beaconProxy_t *const beaconProxyIn, void* userVarIn)
{
	ovr_beaconManager_rpcInterface_t* bmriIn = (ovr_beaconManager_rpcInterface_t*)userVarIn;
	cxa_assert(bmriIn);

	cxa_assert(beaconProxyIn);

	if( !cxa_sntpClient_isClockSet() ) return;

	ovr_beaconUpdate_t* lastUpdate = ovr_beaconProxy_getLastUpdate(beaconProxyIn);
	if( lastUpdate == NULL ) return;

	// get our individual strings together
	char* gatewayUniqueId = cxa_uniqueId_getHexString();

	char timestamp_str[11];
	snprintf(timestamp_str, sizeof(timestamp_str), "%d", cxa_sntpClient_getUnixTimeStamp());
	timestamp_str[sizeof(timestamp_str)-1] = 0;

	cxa_eui48_string_t uuid_str;
	cxa_eui48_toString(ovr_beaconProxy_getEui48(beaconProxyIn), &uuid_str);

	// combine into one payload string
	char notiPayload[UPDATE_MAX_PAYLOAD_BYTES] = "";
	if( !cxa_stringUtils_concat(notiPayload, "{\"gatewayId\":\"", sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, gatewayUniqueId, sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, "\",\"timestamp\":", sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, timestamp_str, sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, ",\"beaconId\":\"", sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, uuid_str.str, sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, "\"}", sizeof(notiPayload)) ) return;

	cxa_mqtt_rpc_node_publishNotification(bmriIn->rpcNode, "onBeaconLost", CXA_MQTT_QOS_ATMOST_ONCE, notiPayload, strlen(notiPayload));
}
