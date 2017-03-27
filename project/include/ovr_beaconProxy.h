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
#include <cxa_eui48.h>

#include <ovr_beaconUpdate.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the ovr_beaconProxy_t object
 */
typedef struct ovr_beaconProxy ovr_beaconProxy_t;


/**
 * @private
 */
struct ovr_beaconProxy
{
	cxa_timeDiff_t td_lastUpdate;

	ovr_beaconUpdate_t lastUpdate;

	ovr_beaconProxy_accelStatus_t cachedAccelStatus;
};


// ******** global function prototypes ********
/**
 * @protected
 */
bool ovr_beaconProxy_init(ovr_beaconProxy_t *const beaconProxyIn, ovr_beaconUpdate_t *const updateIn);


/**
 * @public
 */
ovr_beaconProxy_devType_t ovr_beaconProxy_getDeviceType(ovr_beaconProxy_t *const beaconProxyIn);


/**
 * @public
 */
cxa_eui48_t* ovr_beaconProxy_getEui48(ovr_beaconProxy_t *const beaconProxyIn);


/**
 * @public
 */
ovr_beaconUpdate_t* ovr_beaconProxy_getLastUpdate(ovr_beaconProxy_t *const beaconProxyIn);


/**
 * @public
 * Returns whether the beacon has pending activity. If there is pending activity
 * AND the last update indicated activity, pending activity remains true. If there
 * is pending activity AND the last update indicates no activity, pending activity
 * becomes false.
 *
 * @parameter beaconProxyIn pre-initialize beaconProxy
 *
 * @return whether the beacon has pending activity
 */
ovr_beaconProxy_accelStatus_t ovr_beaconProxy_checkAndResetAccelStatus(ovr_beaconProxy_t *const beaconProxyIn);


/**
 * @protected
 */
void ovr_beaconProxy_update(ovr_beaconProxy_t *const beaconProxyIn, ovr_beaconUpdate_t *const updateIn);


/**
 * @protected
 */
bool ovr_beaconProxy_hasTimedOut(ovr_beaconProxy_t *const beaconProxyIn);

#endif
