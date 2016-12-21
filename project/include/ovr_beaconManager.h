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
#include <cxa_btle_client.h>
#include <cxa_fixedFifo.h>
#include <cxa_ioStream.h>
#include <cxa_logger_header.h>

#include <ovr_beaconProxy.h>
#include <ovr_beaconUpdate.h>


// ******** global macro definitions ********
#ifndef OVR_BEACONMANAGER_MAXNUM_BEACONS
	#define OVR_BEACONMANAGER_MAXNUM_BEACONS		16
#endif

#ifndef OVR_BEACONMANAGER_MAXSIZE_RX_FIFO
	#define OVR_BEACONMANAGER_MAXSIZE_RX_FIFO		4
#endif


// ******** global type definitions *********
typedef struct
{
	bool hasStarted;

	cxa_btle_client_t* btleClient;
	cxa_ioStream_t* ios_remoteClient;

	cxa_array_t knownBeacons;
	ovr_beaconProxy_t knownBeacons_raw[OVR_BEACONMANAGER_MAXNUM_BEACONS];

	cxa_fixedFifo_t rxUpdates;
	ovr_beaconUpdate_t rxUpdates_raw[OVR_BEACONMANAGER_MAXSIZE_RX_FIFO];

	cxa_logger_t logger;
}ovr_beaconManager_t;


// ******** global function prototypes ********
void ovr_beaconManager_init(ovr_beaconManager_t *const bmIn, cxa_btle_client_t *const btleClientIn, cxa_ioStream_t *const ios_remoteClientIn);

#endif
