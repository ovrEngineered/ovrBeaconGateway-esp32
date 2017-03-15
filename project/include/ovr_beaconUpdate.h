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
#include <cxa_eui48.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef enum
{
	OVR_BEACONPROXY_DEVTYPE_UKNOWN = 0,
	OVR_BEACONPROXY_DEVTYPE_BEACON_V1 = 1
}ovr_beaconProxy_devType_t;


typedef struct
{
	bool isCharging;
	bool isEnumerating;
	bool isActive;
}ovr_beaconProxy_status_t;


typedef struct
{
	int8_t rssi_dBm;

	ovr_beaconProxy_devType_t devType;
	cxa_eui48_t uuid;

	ovr_beaconProxy_status_t status;
	uint8_t batt_pcnt100;
	uint8_t currTemp_c;
	uint8_t light_255;
	uint16_t batt_mv;

}ovr_beaconUpdate_t;


// ******** global function prototypes ********
bool ovr_beaconUpdate_init(ovr_beaconUpdate_t *const updateIn, int8_t rssi_dBmIn, cxa_fixedByteBuffer_t *const fbbIn);

bool ovr_beaconUpdate_getIsCharging(ovr_beaconUpdate_t *const updateIn);
bool ovr_beaconUpdate_getIsEnumerating(ovr_beaconUpdate_t *const updateIn);
bool ovr_beaconUpdate_getIsActive(ovr_beaconUpdate_t *const updateIn);

uint8_t ovr_beaconUpdate_getBattery_pcnt100(ovr_beaconUpdate_t *const updateIn);
uint16_t ovr_beaconUpdate_getBattery_mv(ovr_beaconUpdate_t *const updateIn);
int8_t ovr_beaconUpdate_getRssi(ovr_beaconUpdate_t *const updateIn);
uint8_t ovr_beaconUpdate_getTemp_c(ovr_beaconUpdate_t *const updateIn);
uint8_t ovr_beaconUpdate_getLight_255(ovr_beaconUpdate_t *const updateIn);
cxa_eui48_t* ovr_beaconUpdate_getEui48(ovr_beaconUpdate_t *const updateIn);

#endif
