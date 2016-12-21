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
#ifndef OVR_BEACONPROXY_H_
#define OVR_BEACONPROXY_H_


// ******** includes ********
#include <stdbool.h>

#include <cxa_fixedByteBuffer.h>
#include <cxa_timeDiff.h>
#include <cxa_uuid128.h>

#include <ovr_beaconUpdate.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_timeDiff_t td_lastUpdate;

	ovr_beaconUpdate_t lastUpdate;
}ovr_beaconProxy_t;


// ******** global function prototypes ********
bool ovr_beaconProxy_init(ovr_beaconProxy_t *const beaconProxyIn, ovr_beaconUpdate_t *const updateIn);

cxa_uuid128_t* ovr_beaconProxy_getUuid128(ovr_beaconProxy_t *const beaconProxyIn);

void ovr_beaconProxy_update(ovr_beaconProxy_t *const beaconProxyIn, ovr_beaconUpdate_t *const updateIn);

bool ovr_beaconProxy_hasTimedOut(ovr_beaconProxy_t *const beaconProxyIn);

#endif
