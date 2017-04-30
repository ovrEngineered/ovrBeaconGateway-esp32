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
#include "ovr_beaconGateway_rpcInterface.h"


// ******** includes ********
#include <math.h>

#include <cxa_assert.h>
#include <cxa_runLoop.h>
#include <cxa_sntpClient.h>
#include <cxa_stringUtils.h>
#include <cxa_uniqueId.h>

#include <ovr_beaconGateway.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define UPDATE_MAX_PAYLOAD_BYTES				256
#define UPDATE_PERIOD_MS						60000


// ******** local type definitions ********


// ******** local function prototypes ********
static void cb_onRunLoopUpdate(void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void ovr_beaconGateway_rpcInterface_init(ovr_beaconGateway_rpcInterface_t *const bgriIn, ovr_beaconGateway_t *const bgIn, cxa_mqtt_rpc_node_t *const rpcNodeIn)
{
	cxa_assert(bgriIn);
	cxa_assert(bgIn);
	cxa_assert(rpcNodeIn);

	// save our references
	bgriIn->bg = bgIn;
	bgriIn->rpcNode = rpcNodeIn;

	cxa_timeDiff_init(&bgriIn->td_sendUpdate);

	// register for runloop updates
	cxa_runLoop_addEntry(OVR_GW_THREADID_NETWORK, cb_onRunLoopUpdate, (void*)bgriIn);
}


// ******** local function implementations ********
static void cb_onRunLoopUpdate(void* userVarIn)
{
	ovr_beaconGateway_rpcInterface_t* bgriIn = (ovr_beaconGateway_rpcInterface_t*)userVarIn;
	cxa_assert(bgriIn);

	if( cxa_sntpClient_isClockSet() && cxa_timeDiff_isElapsed_recurring_ms(&bgriIn->td_sendUpdate, UPDATE_PERIOD_MS) )
	{
		// get our individual strings together
		char* gatewayUniqueId = cxa_uniqueId_getHexString();

		char timestamp_str[11];
		snprintf(timestamp_str, sizeof(timestamp_str), "%d", cxa_sntpClient_getUnixTimeStamp());
		timestamp_str[sizeof(timestamp_str)-1] = 0;

		char isRadioReady_str[2];
		snprintf(isRadioReady_str, sizeof(isRadioReady_str), "%d", ovr_beaconGateway_isBeaconRadioReady(bgriIn->bg));
		isRadioReady_str[sizeof(isRadioReady_str)-1] = 0;

		char temp_str[8];
		snprintf(temp_str, sizeof(temp_str), "%.02f", ovr_beaconGateway_getLastTemp_degC(bgriIn->bg));
		temp_str[sizeof(temp_str)-1] = 0;

		char light_str[4];
		snprintf(light_str, sizeof(light_str), "%d", ovr_beaconGateway_getLastLight_255(bgriIn->bg));
		light_str[sizeof(light_str)-1] = 0;

		// combine into one payload string
		char notiPayload[UPDATE_MAX_PAYLOAD_BYTES] = "";
		if( !cxa_stringUtils_concat(notiPayload, "{\"gatewayId\":\"", sizeof(notiPayload)) ) return;
		if( !cxa_stringUtils_concat(notiPayload, gatewayUniqueId, sizeof(notiPayload)) ) return;
		if( !cxa_stringUtils_concat(notiPayload, "\",\"timestamp\":", sizeof(notiPayload)) ) return;
		if( !cxa_stringUtils_concat(notiPayload, timestamp_str, sizeof(notiPayload)) ) return;
		if( !cxa_stringUtils_concat(notiPayload, ",\"isBeaconRadioReady\":", sizeof(notiPayload)) ) return;
		if( !cxa_stringUtils_concat(notiPayload, isRadioReady_str, sizeof(notiPayload)) ) return;

		if( ovr_beaconGateway_isBeaconRadioReady(bgriIn->bg) )
		{
			if( !cxa_stringUtils_concat(notiPayload, ",\"temp_c\":", sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, temp_str, sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, ",\"light_255\":", sizeof(notiPayload)) ) return;
			if( !cxa_stringUtils_concat(notiPayload, light_str, sizeof(notiPayload)) ) return;
		}


		if( !cxa_stringUtils_concat(notiPayload, "}", sizeof(notiPayload)) ) return;

		cxa_mqtt_rpc_node_publishNotification(bgriIn->rpcNode, "onBeaconGatewayUpdate", CXA_MQTT_QOS_ATMOST_ONCE, notiPayload, strlen(notiPayload));
	}
}
