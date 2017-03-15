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
#include "ovr_beaconUpdate.h"


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
bool ovr_beaconUpdate_init(ovr_beaconUpdate_t *const updateIn, int8_t rssi_dBmIn, cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(updateIn);
	cxa_assert(fbbIn);

	updateIn->rssi_dBm = rssi_dBmIn;

	uint8_t devType_raw;
	if( !cxa_fixedByteBuffer_get_uint8(fbbIn, 0, devType_raw) ) return false;
	updateIn->devType = devType_raw;

	if( !cxa_eui48_initFromBuffer(&updateIn->uuid, fbbIn, 1) ) return false;

	uint8_t status_raw;
	if( !cxa_fixedByteBuffer_get_uint8(fbbIn, 7, status_raw) ) return false;
	updateIn->status.isCharging = status_raw & (1 << 7);
	updateIn->status.isEnumerating = status_raw & (1 << 6);
	updateIn->status.isActive = status_raw & (1 << 5);

	if( !cxa_fixedByteBuffer_get_uint8(fbbIn, 8, updateIn->batt_pcnt100) ) return false;
	if( !cxa_fixedByteBuffer_get_uint8(fbbIn, 9, updateIn->currTemp_c) ) return false;
	if( !cxa_fixedByteBuffer_get_uint8(fbbIn, 10, updateIn->light_255) ) return false;
	if( !cxa_fixedByteBuffer_get_uint16LE(fbbIn, 11, updateIn->batt_mv) ) return false;

	return true;
}


bool ovr_beaconUpdate_getIsCharging(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return updateIn->status.isCharging;
}


bool ovr_beaconUpdate_getIsEnumerating(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return updateIn->status.isEnumerating;
}


bool ovr_beaconUpdate_getIsActive(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return updateIn->status.isActive;
}


void ovr_beaconUpdate_setIsActive(ovr_beaconUpdate_t *const updateIn, bool isActiveIn)
{
	cxa_assert(updateIn);

	updateIn->status.isActive = isActiveIn;
}


uint8_t ovr_beaconUpdate_getBattery_pcnt100(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return updateIn->batt_pcnt100;
}


uint16_t ovr_beaconUpdate_getBattery_mv(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return updateIn->batt_mv;
}


int8_t ovr_beaconUpdate_getRssi(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return updateIn->rssi_dBm;
}


uint8_t ovr_beaconUpdate_getTemp_c(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return updateIn->currTemp_c;
}


uint8_t ovr_beaconUpdate_getLight_255(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return updateIn->light_255;
}


cxa_eui48_t* ovr_beaconUpdate_getEui48(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return &updateIn->uuid;
}


// ******** local function implementations ********
