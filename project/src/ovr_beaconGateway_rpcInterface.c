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
#define CHECKIN_PERIOD_MS					60000


// ******** local type definitions ********


// ******** local function prototypes ********
static void cb_onRunLoopUpdate(void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void ovr_beaconGateway_rpcInterface_init(ovr_beaconGateway_rpcInterface_t *const bgriIn, ovr_beaconGateway_t *const bgIn, cxa_mqtt_rpc_node_t *const rootNodeIn)
{
	cxa_assert(bgriIn);
	cxa_assert(bgIn);
	cxa_assert(rootNodeIn);

	// save our references
	bgriIn->bg = bgIn;
	bgriIn->rpcNode_root = rootNodeIn;

	cxa_timeDiff_init(&bgriIn->td_sendCheckin);

	// initialize our RPC nodes
	cxa_mqtt_rpc_node_init_formattedString(&bgriIn->rpcNode_ambient, bgriIn->rpcNode_root, "ambient");
	cxa_mqtt_rpc_node_init_formattedString(&bgriIn->rpcNode_ambient_light, &bgriIn->rpcNode_ambient, "light_255");
	cxa_mqtt_rpc_node_init_formattedString(&bgriIn->rpcNode_ambient_temp, &bgriIn->rpcNode_ambient, "temp_c");

	// register for runloop updates
	cxa_runLoop_addEntry(OVR_GW_THREADID_NETWORK, cb_onRunLoopUpdate, (void*)bgriIn);

}


void ovr_beaconGateway_rpcInterface_notifyTempChanged(ovr_beaconGateway_rpcInterface_t *const bgriIn, float newTemp_degCIn)
{
	cxa_assert(bgriIn);

	char timestamp_str[11];
	snprintf(timestamp_str, sizeof(timestamp_str), "%d", cxa_sntpClient_getUnixTimeStamp());
	timestamp_str[sizeof(timestamp_str)-1] = 0;

	char value_str[8];
	snprintf(value_str, sizeof(value_str), "%.02f", newTemp_degCIn);
	value_str[sizeof(value_str)-1] = 0;

	char notiPayload[UPDATE_MAX_PAYLOAD_BYTES] = "";
	if( !cxa_stringUtils_concat(notiPayload, "{\"timestamp_s_local\":", sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, timestamp_str, sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, ",\"value_num\":", sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, value_str, sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, "}", sizeof(notiPayload)) ) return;

	cxa_mqtt_rpc_node_publishNotification(&bgriIn->rpcNode_ambient_temp, "onChange", CXA_MQTT_QOS_ATMOST_ONCE, notiPayload, strlen(notiPayload));
}


void ovr_beaconGateway_rpcInterface_notifyLightChanged(ovr_beaconGateway_rpcInterface_t *const bgriIn, uint8_t newLight_255In)
{
	cxa_assert(bgriIn);

	char timestamp_str[11];
	snprintf(timestamp_str, sizeof(timestamp_str), "%d", cxa_sntpClient_getUnixTimeStamp());
	timestamp_str[sizeof(timestamp_str)-1] = 0;

	char value_str[4];
	snprintf(value_str, sizeof(value_str), "%d", newLight_255In);
	value_str[sizeof(value_str)-1] = 0;

	char notiPayload[UPDATE_MAX_PAYLOAD_BYTES] = "";
	if( !cxa_stringUtils_concat(notiPayload, "{\"timestamp_s_local\":", sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, timestamp_str, sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, ",\"value_num\":", sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, value_str, sizeof(notiPayload)) ) return;
	if( !cxa_stringUtils_concat(notiPayload, "}", sizeof(notiPayload)) ) return;

	cxa_mqtt_rpc_node_publishNotification(&bgriIn->rpcNode_ambient_light, "onChange", CXA_MQTT_QOS_ATMOST_ONCE, notiPayload, strlen(notiPayload));
}


// ******** local function implementations ********
static void cb_onRunLoopUpdate(void* userVarIn)
{
	ovr_beaconGateway_rpcInterface_t* bgriIn = (ovr_beaconGateway_rpcInterface_t*)userVarIn;
	cxa_assert(bgriIn);

	if( cxa_timeDiff_isElapsed_recurring_ms(&bgriIn->td_sendCheckin, CHECKIN_PERIOD_MS) )
	{
		char timestamp_str[11];
		snprintf(timestamp_str, sizeof(timestamp_str), "%d", cxa_sntpClient_getUnixTimeStamp());
		timestamp_str[sizeof(timestamp_str)-1] = 0;

		char variant_str[2];
		snprintf(variant_str, sizeof(variant_str), "%d", ovr_beaconGateway_getVariant(bgriIn->bg));
		variant_str[sizeof(variant_str)-1] = 0;

		char isRadioReady_str[2];
		snprintf(isRadioReady_str, sizeof(isRadioReady_str), "%d", ovr_beaconGateway_isBeaconRadioReady(bgriIn->bg));
		isRadioReady_str[sizeof(isRadioReady_str)-1] = 0;

		// combine into one payload string
		char notiPayload[UPDATE_MAX_PAYLOAD_BYTES] = "";
		if( !cxa_stringUtils_concat(notiPayload, "{\"variant\":", sizeof(notiPayload)) ) return;
		if( !cxa_stringUtils_concat(notiPayload, variant_str, sizeof(notiPayload)) ) return;
		if( !cxa_stringUtils_concat(notiPayload, ",\"timestamp_s_local\":", sizeof(notiPayload)) ) return;
		if( !cxa_stringUtils_concat(notiPayload, timestamp_str, sizeof(notiPayload)) ) return;
		if( !cxa_stringUtils_concat(notiPayload, ",\"isBeaconRadioReady\":", sizeof(notiPayload)) ) return;
		if( !cxa_stringUtils_concat(notiPayload, isRadioReady_str, sizeof(notiPayload)) ) return;
		if( !cxa_stringUtils_concat(notiPayload, "}", sizeof(notiPayload)) ) return;

		cxa_mqtt_rpc_node_publishNotification(bgriIn->rpcNode_root, "checkIn", CXA_MQTT_QOS_ATMOST_ONCE, notiPayload, strlen(notiPayload));
	}
}
