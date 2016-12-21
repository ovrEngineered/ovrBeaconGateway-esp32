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
#ifndef OVR_BEACONUPDATE_H_
#define OVR_BEACONUPDATE_H_


// ******** includes ********
#include <stdint.h>
#include <cxa_uuid128.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef enum
{
	OVR_BEACONPROXY_DEVTYPE_UKNOWN = 0,
	OVR_BEACONPROXY_DEVTYPE_BEACON_V1 = 1
}ovr_beaconProxy_devType_t;


typedef struct
{
	uint8_t isEnumerating;
	uint8_t needsPoll;
}ovr_beaconProxy_status_t;


typedef struct
{
	int8_t rssi_dBm;

	ovr_beaconProxy_devType_t devType;
	cxa_uuid128_t uuid;

	uint8_t txPower_1m;

	ovr_beaconProxy_status_t status;
	uint8_t batt_pcnt;
	uint8_t currTemp_c;
}ovr_beaconUpdate_t;


// ******** global function prototypes ********
bool ovr_beaconUpdate_init(ovr_beaconUpdate_t *const updateIn, int8_t rssi_dBmIn, cxa_fixedByteBuffer_t *const fbbIn);

cxa_uuid128_t* ovr_beaconUpdate_getUuid128(ovr_beaconUpdate_t *const updateIn);

#endif
