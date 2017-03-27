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


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


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

	beaconProxyIn->cachedAccelStatus = ovr_beaconUpdate_getAccelStatus(&beaconProxyIn->lastUpdate);

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


ovr_beaconProxy_accelStatus_t ovr_beaconProxy_checkAndResetAccelStatus(ovr_beaconProxy_t *const beaconProxyIn)
{
	cxa_assert(beaconProxyIn);

	ovr_beaconProxy_accelStatus_t cachedStatus = beaconProxyIn->cachedAccelStatus;

	ovr_beaconUpdate_t* lastUpdate = ovr_beaconProxy_getLastUpdate(beaconProxyIn);
	if( lastUpdate != NULL )
	{
		beaconProxyIn->cachedAccelStatus = ovr_beaconUpdate_getAccelStatus(lastUpdate);
	}

	return cachedStatus;
}


void ovr_beaconProxy_update(ovr_beaconProxy_t *const beaconProxyIn, ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(beaconProxyIn);
	cxa_assert(updateIn);

	memcpy(&beaconProxyIn->lastUpdate, updateIn, sizeof(beaconProxyIn->lastUpdate));
	cxa_timeDiff_setStartTime_now(&beaconProxyIn->td_lastUpdate);

	// latch each status bit to 1 if needed
	ovr_beaconProxy_accelStatus_t newStatus = ovr_beaconUpdate_getAccelStatus(updateIn);
	if( !beaconProxyIn->cachedAccelStatus.hasOccurred_1tap ) beaconProxyIn->cachedAccelStatus.hasOccurred_1tap = newStatus.hasOccurred_1tap;
	if( !beaconProxyIn->cachedAccelStatus.hasOccurred_2tap ) beaconProxyIn->cachedAccelStatus.hasOccurred_2tap = newStatus.hasOccurred_2tap;
	if( !beaconProxyIn->cachedAccelStatus.hasOccurred_activity ) beaconProxyIn->cachedAccelStatus.hasOccurred_activity = newStatus.hasOccurred_activity;
	if( !beaconProxyIn->cachedAccelStatus.hasOccurred_freeFall ) beaconProxyIn->cachedAccelStatus.hasOccurred_freeFall = newStatus.hasOccurred_freeFall;
}


bool ovr_beaconProxy_hasTimedOut(ovr_beaconProxy_t *const beaconProxyIn)
{
	cxa_assert(beaconProxyIn);

	return cxa_timeDiff_isElapsed_ms(&beaconProxyIn->td_lastUpdate, OVR_BEACONPROXY_LOSTTIMEOUT_MS);
}


// ******** local function implementations ********
