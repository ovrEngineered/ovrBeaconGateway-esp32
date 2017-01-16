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
#ifndef OVR_BEACONMANAGER_RPCINTERFACE_H_
#define OVR_BEACONMANAGER_RPCINTERFACE_H_


// ******** includes ********
#include <cxa_mqtt_rpc_node.h>
#include <cxa_timeDiff.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct ovr_beaconManager_rpcInterface ovr_beaconManager_rpcInterface_t;


// forward declaration to avoid include cycle
typedef struct ovr_beaconManager ovr_beaconManager_t;


/**
 * @private
 */
struct ovr_beaconManager_rpcInterface
{
	ovr_beaconManager_t* bm;
	cxa_mqtt_rpc_node_t* rpcNode;

	cxa_timeDiff_t td_sendUpdate;
};


// ******** global function prototypes ********
void ovr_beaconManager_rpcInterface_init(ovr_beaconManager_rpcInterface_t *const bmriIn, ovr_beaconManager_t *const bmIn, cxa_mqtt_rpc_node_t *const rpcNodeIn);


#endif
