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
#ifndef OVR_BEACONGATEWAY_RPCINTERFACE_H_
#define OVR_BEACONGATEWAY_RPCINTERFACE_H_


// ******** includes ********
#include <cxa_mqtt_rpc_node.h>
#include <cxa_timeDiff.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct ovr_beaconGateway_rpcInterface ovr_beaconGateway_rpcInterface_t;


// forward declaration to avoid include cycle
typedef struct ovr_beaconGateway ovr_beaconGateway_t;


/**
 * @private
 */
struct ovr_beaconGateway_rpcInterface
{
	ovr_beaconGateway_t* bg;

	cxa_mqtt_rpc_node_t* rpcNode_root;
	cxa_mqtt_rpc_node_t rpcNode_ambient;
	cxa_mqtt_rpc_node_t rpcNode_ambient_temp;
	cxa_mqtt_rpc_node_t rpcNode_ambient_light;

	cxa_timeDiff_t td_sendCheckin;
};


// ******** global function prototypes ********
void ovr_beaconGateway_rpcInterface_init(ovr_beaconGateway_rpcInterface_t *const bgriIn, ovr_beaconGateway_t *const bgIn, cxa_mqtt_rpc_node_t *const rootNodeIn);

void ovr_beaconGateway_rpcInterface_notifyTempChanged(ovr_beaconGateway_rpcInterface_t *const bgriIn, float newTemp_degCIn);
void ovr_beaconGateway_rpcInterface_notifyLightChanged(ovr_beaconGateway_rpcInterface_t *const bgriIn, uint8_t newLight_255In);

#endif
