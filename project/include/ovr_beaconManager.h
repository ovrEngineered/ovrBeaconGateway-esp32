/**
 * @file
 * @copyright 2016 opencxa.org
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
#ifndef OVR_BEACONMANAGER_H_
#define OVR_BEACONMANAGER_H_


// ******** includes ********
#include <cxa_array.h>
#include <cxa_config.h>
#include <cxa_btle_client.h>
#include <cxa_fixedFifo.h>
#include <cxa_ioStream.h>
#include <cxa_logger_header.h>
#include <cxa_timeDiff.h>

#include <ovr_beaconManager_rpcInterface.h>
#include <ovr_beaconProxy.h>
#include <ovr_beaconUpdate.h>


// ******** global macro definitions ********
#ifndef OVR_BEACONMANAGER_MAXNUM_BEACONS
	#define OVR_BEACONMANAGER_MAXNUM_BEACONS		16
#endif

#ifndef OVR_BEACONMANAGER_MAXSIZE_RX_FIFO
	#define OVR_BEACONMANAGER_MAXSIZE_RX_FIFO		4
#endif

#ifndef OVR_BEACONMANAGER_MAXNUM_LISTENERS
	#define OVR_BEACONMANAGER_MAXNUM_LISTENERS			4
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct ovr_beaconManager ovr_beaconManager_t;


/**
 * @public
 */
typedef void (*ovr_beaconManager_cb_beaconListener_t)(ovr_beaconProxy_t *const beaconProxyIn, void* userVarIn);


/**
 * @private
 */
typedef struct
{
	ovr_beaconManager_cb_beaconListener_t cb_onBeaconFound;
	ovr_beaconManager_cb_beaconListener_t cb_onBeaconUpdate;
	ovr_beaconManager_cb_beaconListener_t cb_onBeaconLost;

	void *userVar;
}ovr_beaconManager_listenerEntry_t;


/**
 * @private
 */
struct ovr_beaconManager
{
	cxa_timeDiff_t td_scanningCheck;

	cxa_btle_client_t* btleClient;
	ovr_beaconManager_rpcInterface_t bmri;

	cxa_array_t knownBeacons;
	ovr_beaconProxy_t knownBeacons_raw[OVR_BEACONMANAGER_MAXNUM_BEACONS];

	cxa_fixedFifo_t rxUpdates;
	ovr_beaconUpdate_t rxUpdates_raw[OVR_BEACONMANAGER_MAXSIZE_RX_FIFO];

	cxa_array_t listeners;
	ovr_beaconManager_listenerEntry_t listeners_raw[OVR_BEACONMANAGER_MAXNUM_LISTENERS];

	cxa_logger_t logger;
};


// ******** global function prototypes ********
void ovr_beaconManager_init(ovr_beaconManager_t *const bmIn, cxa_btle_client_t *const btleClientIn, cxa_mqtt_rpc_node_t *const rpcNodeIn);

void ovr_beaconManager_addListener(ovr_beaconManager_t *const bmIn,
		ovr_beaconManager_cb_beaconListener_t cb_onBeaconFoundIn,
		ovr_beaconManager_cb_beaconListener_t cb_onBeaconUpdateIn,
		ovr_beaconManager_cb_beaconListener_t cb_onBeaconLostIn,
		void* userVarIn);

cxa_array_t* ovr_beaconManager_getKnownBeacons(ovr_beaconManager_t *const bmIn);

#endif
