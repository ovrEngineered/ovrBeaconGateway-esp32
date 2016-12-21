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
#include "ovr_beaconManager.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_runLoop.h>

#include <ovr_beaconProxy.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define COMPANY_ID				0xFFFF


// ******** local type definitions ********


// ******** local function prototypes ********
static void processRxUpdateFifo(ovr_beaconManager_t *const bmIn);
static void pruneLostProxies(ovr_beaconManager_t *const bmIn);

static void cb_onRunLoopUpdate(void* userVarIn);
static void btleCb_onAdvertRx(cxa_btle_advPacket_t* packetIn, void* userVarIn);

static cxa_btle_advField_t* findBeaconFieldInPacket(cxa_btle_advPacket_t* packetIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void ovr_beaconManager_init(ovr_beaconManager_t *const bmIn, cxa_btle_client_t *const btleClientIn, cxa_ioStream_t *const ios_remoteClientIn)
{
	cxa_assert(bmIn);
	cxa_assert(btleClientIn);

	// setup our internal state
	bmIn->hasStarted = false;
	bmIn->btleClient = btleClientIn;
	bmIn->ios_remoteClient = ios_remoteClientIn;

	// initialize our logger
	cxa_logger_init(&bmIn->logger, "beaconManager");

	cxa_array_initStd(&bmIn->knownBeacons, bmIn->knownBeacons_raw);
	cxa_fixedFifo_initStd(&bmIn->rxUpdates, CXA_FF_ON_FULL_DROP, bmIn->rxUpdates_raw);

	// add ourselves to the runloop
	cxa_runLoop_addEntry(cb_onRunLoopUpdate, (void*)bmIn);
}


// ******** local function implementations ********
static void processRxUpdateFifo(ovr_beaconManager_t *const bmIn)
{
	cxa_assert(bmIn);

	ovr_beaconUpdate_t* updates = NULL;
	size_t numUpdates = cxa_fixedFifo_bulkDequeue_peek(&bmIn->rxUpdates, (void**)&updates);
	for( size_t i = 0; i < numUpdates; i++ )
	{
		ovr_beaconUpdate_t* currUpdate = &updates[i];

		// search for this proxy in our known proxy list
		bool isKnownProxy = false;
		cxa_array_iterate(&bmIn->knownBeacons, currProxy, ovr_beaconProxy_t)
		{
			if( currProxy == NULL ) continue;

			cxa_uuid128_string_t uuid_str_proxy, uuid_str_update;
			cxa_uuid128_toString(ovr_beaconProxy_getUuid128(currProxy), &uuid_str_proxy);
			cxa_uuid128_toString(ovr_beaconUpdate_getUuid128(currUpdate), &uuid_str_update);

			if( cxa_uuid128_isEqual(ovr_beaconProxy_getUuid128(currProxy), ovr_beaconUpdate_getUuid128(currUpdate)) )
			{
				isKnownProxy = true;
				ovr_beaconProxy_update(currProxy, currUpdate);

				cxa_uuid128_string_t uuid_str;
				cxa_uuid128_toShortString(ovr_beaconProxy_getUuid128(currProxy), &uuid_str);
				cxa_logger_debug(&bmIn->logger, "updated proxy '%s'  rssi: %d  temp_f: %d  batt_pcnt: %d",
						uuid_str.str, currUpdate->rssi_dBm, (int8_t)((float)currUpdate->currTemp_c * 1.8 + 32.0), currUpdate->batt_pcnt);
				break;
			}
		}
		if( isKnownProxy ) continue;

		// if we made it here, we have a new proxy
		ovr_beaconProxy_t newProxy;
		if( !ovr_beaconProxy_init(&newProxy, currUpdate) ) return;
		cxa_array_append(&bmIn->knownBeacons, &newProxy);

		cxa_uuid128_string_t uuid_str;
		cxa_uuid128_toString(ovr_beaconProxy_getUuid128(&newProxy), &uuid_str);
		cxa_logger_debug(&bmIn->logger, "new proxy '%s'", uuid_str.str);

	}
	cxa_fixedFifo_bulkDequeue(&bmIn->rxUpdates, numUpdates);
}


static void pruneLostProxies(ovr_beaconManager_t *const bmIn)
{
	cxa_assert(bmIn);

	// can't remove whilst iterating...so setup a temporary array
	cxa_array_t timedOutProxies;
	ovr_beaconProxy_t* timedOutProxies_raw[cxa_array_getMaxSize_elems(&bmIn->knownBeacons)];
	cxa_array_initStd(&timedOutProxies, timedOutProxies_raw);

	// iterate through our beacons and see if we've "lost" any
	cxa_array_iterate(&bmIn->knownBeacons, currProxy, ovr_beaconProxy_t)
	{
		if( currProxy == NULL ) continue;

		if( ovr_beaconProxy_hasTimedOut(currProxy) )
		{
			cxa_uuid128_string_t uuid_str;
			cxa_uuid128_toString(ovr_beaconProxy_getUuid128(currProxy), &uuid_str);
			cxa_logger_debug(&bmIn->logger, "lost proxy '%s'", uuid_str.str);

			cxa_array_append(&timedOutProxies, &currProxy);
		}
	}

	// now do the actual removal
	cxa_array_iterate(&timedOutProxies, currProxyPtr, ovr_beaconProxy_t*)
	{
		if( currProxyPtr == NULL ) continue;

		cxa_array_remove(&bmIn->knownBeacons, *currProxyPtr);
	}
}


static void cb_onRunLoopUpdate(void* userVarIn)
{
	ovr_beaconManager_t* bmIn = (ovr_beaconManager_t*)userVarIn;
	cxa_assert(bmIn);

	if( !bmIn->hasStarted )
	{
		bmIn->hasStarted = true;
		cxa_btle_client_startScan_passive(bmIn->btleClient, btleCb_onAdvertRx, NULL, (void*)bmIn);
	}

	// do the real business
	processRxUpdateFifo(bmIn);
	pruneLostProxies(bmIn);
}


static void btleCb_onAdvertRx(cxa_btle_advPacket_t* packetIn, void* userVarIn)
{
	ovr_beaconManager_t* bmIn = (ovr_beaconManager_t*)userVarIn;
	cxa_assert(bmIn);

	// see if this packet is a beacon packet
	cxa_btle_advField_t* beaconField = findBeaconFieldInPacket(packetIn);
	if( beaconField == NULL ) return;

//	cxa_logger_stepDebug_memDump("data: ",
//								 cxa_fixedByteBuffer_get_pointerToIndex(&beaconField->asManufacturerData.manBytes, 0),
//								 cxa_fixedByteBuffer_getSize_bytes(&beaconField->asManufacturerData.manBytes));

	// parse our information from the packet
	ovr_beaconUpdate_t parsedUpdate;
	if( !ovr_beaconUpdate_init(&parsedUpdate, packetIn->rssi, &beaconField->asManufacturerData.manBytes) ) return;

	// send it to the runLoop for processing...
	cxa_fixedFifo_queue(&bmIn->rxUpdates, &parsedUpdate);
}


static cxa_btle_advField_t* findBeaconFieldInPacket(cxa_btle_advPacket_t* packetIn)
{
	cxa_assert(packetIn);

	cxa_array_iterate(&packetIn->advFields, currField, cxa_btle_advField_t)
	{
		if( currField == NULL ) continue;

		if( (currField->type == CXA_BTLE_ADVFIELDTYPE_MAN_DATA) && (currField->asManufacturerData.companyId == COMPANY_ID) )
		{
			return currField;
		}
	}

	return NULL;
}
