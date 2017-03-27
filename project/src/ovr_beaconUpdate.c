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

	if( !cxa_fixedByteBuffer_get_uint8(fbbIn, 7, updateIn->status_raw) ) return false;
	updateIn->devStatus.isCharging = updateIn->status_raw & (1 << 7);
	updateIn->devStatus.isEnumerating = updateIn->status_raw & (1 << 6);
	updateIn->devStatus.accelError = updateIn->status_raw & (1 << 5);
	updateIn->devStatus.tempError = updateIn->status_raw & (1 << 4);
	updateIn->devStatus.lightError = updateIn->status_raw & (1 << 3);
	updateIn->devStatus.isAccelEnabled = updateIn->status_raw & (1 << 2);
	updateIn->devStatus.isTempEnabled = updateIn->status_raw & (1 << 1);
	updateIn->devStatus.isLightEnabled = updateIn->status_raw & (1 << 0);

	if( !cxa_fixedByteBuffer_get_uint8(fbbIn, 8, updateIn->batt_pcnt100) ) return false;
	if( !cxa_fixedByteBuffer_get_uint16LE(fbbIn, 9, updateIn->currTemp_deciDegC) ) return false;
	if( !cxa_fixedByteBuffer_get_uint8(fbbIn, 11, updateIn->light_255) ) return false;

	if( !cxa_fixedByteBuffer_get_uint8(fbbIn, 12, updateIn->accelStatus_raw) ) return false;
	updateIn->accelStatus.hasOccurred_freeFall = updateIn->accelStatus_raw & (1 << 3);
	updateIn->accelStatus.hasOccurred_2tap = updateIn->accelStatus_raw & (1 << 2);
	updateIn->accelStatus.hasOccurred_1tap = updateIn->accelStatus_raw & (1 << 1);
	updateIn->accelStatus.hasOccurred_activity = updateIn->accelStatus_raw & (1 << 0);

	if( !cxa_fixedByteBuffer_get_uint16LE(fbbIn, 13, updateIn->batt_mv) ) return false;

	return true;
}


bool ovr_beaconUpdate_getIsCharging(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return updateIn->devStatus.isCharging;
}


bool ovr_beaconUpdate_getIsEnumerating(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return updateIn->devStatus.isEnumerating;
}


bool ovr_beaconUpdate_hasError(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return updateIn->devStatus.accelError | updateIn->devStatus.tempError | updateIn->devStatus.lightError;
}


ovr_beaconProxy_deviceStatus_t ovr_beaconUpdate_getDeviceStatus(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return updateIn->devStatus;
}


uint8_t ovr_beaconUpdate_getStatusByte(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return updateIn->status_raw;
}


ovr_beaconProxy_accelStatus_t ovr_beaconUpdate_getAccelStatus(ovr_beaconUpdate_t *const updateIn)
{
	return updateIn->accelStatus;
}


uint8_t ovr_beaconUpdate_getBattery_pcnt100(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return updateIn->batt_pcnt100;
}


float ovr_beaconUpdate_getBattery_v(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return ((float)updateIn->batt_mv) / 1000.0;
}


int8_t ovr_beaconUpdate_getRssi(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return updateIn->rssi_dBm;
}


float ovr_beaconUpdate_getTemp_c(ovr_beaconUpdate_t *const updateIn)
{
	cxa_assert(updateIn);

	return updateIn->currTemp_deciDegC / 10.0;
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
