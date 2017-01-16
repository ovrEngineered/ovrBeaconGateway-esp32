/**
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
#include "ovr_beaconProxy.h"


// ******** includes ********
#include <string.h>

#include <cxa_assert.h>
#include <cxa_config.h>
#include <cxa_criticalSection.h>


// ******** local macro definitions ********
#ifndef OVR_BEACONPROXY_LOSTTIMEOUT_MS
	#define OVR_BEACONPROXY_LOSTTIMEOUT_MS			60000
#endif


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
bool ovr_beaconProxy_init(ovr_beaconProxy_t *const beaconProxyIn, ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(beaconProxyIn);
	cxa_assert(updateIn);

	// save our update and update our pointers
	// pointers shouldn't change...even after updates
	memcpy(&beaconProxyIn->lastUpdate, updateIn, sizeof(beaconProxyIn->lastUpdate));

	// last but not least, start our timeDiff
	cxa_timeDiff_init(&beaconProxyIn->td_lastUpdate);

	return true;
}


ovr_beaconProxy_devType_t ovr_beaconProxy_getDeviceType(ovr_beaconProxy_t *const beaconProxyIn)
{
	cxa_assert(beaconProxyIn);

	return beaconProxyIn->lastUpdate.devType;
}


cxa_eui48_t* ovr_beaconProxy_getEui48(ovr_beaconProxy_t *const beaconProxyIn)
{
	cxa_assert(beaconProxyIn);

	return &beaconProxyIn->lastUpdate.uuid;
}


ovr_beaconUpdate_t* ovr_beaconProxy_getLastUpdate(ovr_beaconProxy_t *const beaconProxyIn)
{
	cxa_assert(beaconProxyIn);

	return &beaconProxyIn->lastUpdate;
}


void ovr_beaconProxy_update(ovr_beaconProxy_t *const beaconProxyIn, ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(beaconProxyIn);
	cxa_assert(updateIn);

	memcpy(&beaconProxyIn->lastUpdate, updateIn, sizeof(beaconProxyIn->lastUpdate));
	cxa_timeDiff_setStartTime_now(&beaconProxyIn->td_lastUpdate);
}


bool ovr_beaconProxy_hasTimedOut(ovr_beaconProxy_t *const beaconProxyIn)
{
	cxa_assert(beaconProxyIn);

	return cxa_timeDiff_isElapsed_ms(&beaconProxyIn->td_lastUpdate, OVR_BEACONPROXY_LOSTTIMEOUT_MS);
}


// ******** local function implementations ********
