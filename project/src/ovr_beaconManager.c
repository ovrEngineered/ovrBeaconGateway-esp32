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
#define SCAN_CHECK_PERIOD_MS	10000


// ******** local type definitions ********


// ******** local function prototypes ********
static void processRxUpdateFifo(ovr_beaconManager_t *const bmIn);
static void pruneLostProxies(ovr_beaconManager_t *const bmIn);

static void notifyListeners_onFound(ovr_beaconManager_t *const bmIn, ovr_beaconProxy_t *const beaconProxyIn);
static void notifyListeners_onUpdate(ovr_beaconManager_t *const bmIn, ovr_beaconProxy_t *const beaconProxyIn);
static void notifyListeners_onLost(ovr_beaconManager_t *const bmIn, ovr_beaconProxy_t *const beaconProxyIn);

static void cb_onRunLoopUpdate(void* userVarIn);

static void btleCb_onReady(cxa_btle_client_t *const btlecIn, void* userVarIn);
static void btleCb_onFailedInit(cxa_btle_client_t *const btlecIn, bool willAutoRetryIn, void* userVarIn);
static void btleCb_onScanStart(void* userVarIn, bool wasSuccessfulIn);
static void btleCb_onAdvertRx(cxa_btle_advPacket_t* packetIn, void* userVarIn);

static cxa_btle_advField_t* findBeaconFieldInPacket(cxa_btle_advPacket_t* packetIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void ovr_beaconManager_init(ovr_beaconManager_t *const bmIn, cxa_btle_client_t *const btleClientIn, cxa_mqtt_rpc_node_t *const rpcNodeIn)
{
	cxa_assert(bmIn);
	cxa_assert(btleClientIn);

	// setup our internal state
	cxa_timeDiff_init(&bmIn->td_scanningCheck);

	// initialize our logger
	cxa_logger_init(&bmIn->logger, "beaconManager");

	cxa_array_initStd(&bmIn->knownBeacons, bmIn->knownBeacons_raw);
	cxa_fixedFifo_initStd(&bmIn->rxUpdates, CXA_FF_ON_FULL_DROP, bmIn->rxUpdates_raw);
	cxa_array_initStd(&bmIn->listeners, bmIn->listeners_raw);

	// setup our BTLE
	bmIn->btleClient = btleClientIn;
	cxa_btle_client_addListener(bmIn->btleClient, btleCb_onReady, btleCb_onFailedInit, (void*)bmIn);

	// setup rpc if needed
	if( rpcNodeIn != NULL ) ovr_beaconManager_rpcInterface_init(&bmIn->bmri, bmIn, rpcNodeIn);

	// add ourselves to the runloop
	cxa_runLoop_addEntry(cb_onRunLoopUpdate, (void*)bmIn);
}


void ovr_beaconManager_addListener(ovr_beaconManager_t *const bmIn,
		ovr_beaconManager_cb_beaconListener_t cb_onBeaconFoundIn,
		ovr_beaconManager_cb_beaconListener_t cb_onBeaconUpdateIn,
		ovr_beaconManager_cb_beaconListener_t cb_onBeaconLostIn,
		void* userVarIn)
{
	cxa_assert(bmIn);

	// create our new listener
	ovr_beaconManager_listenerEntry_t newEntry = {
			.cb_onBeaconFound = cb_onBeaconFoundIn,
			.cb_onBeaconUpdate = cb_onBeaconUpdateIn,
			.cb_onBeaconLost = cb_onBeaconLostIn,
			.userVar = userVarIn
	};

	// add it to our array of listeners
	cxa_assert( cxa_array_append(&bmIn->listeners, &newEntry) );
}


cxa_array_t* ovr_beaconManager_getKownBeacons(ovr_beaconManager_t *const bmIn)
{
	cxa_assert(bmIn);

	return &bmIn->knownBeacons;
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

			cxa_eui48_string_t uuid_str_proxy, uuid_str_update;
			cxa_eui48_toString(ovr_beaconProxy_getEui48(currProxy), &uuid_str_proxy);
			cxa_eui48_toString(ovr_beaconUpdate_getEui48(currUpdate), &uuid_str_update);

			if( cxa_eui48_isEqual(ovr_beaconProxy_getEui48(currProxy), ovr_beaconUpdate_getEui48(currUpdate)) )
			{
				isKnownProxy = true;
				ovr_beaconProxy_update(currProxy, currUpdate);

				cxa_eui48_string_t uuid_str;
				cxa_eui48_toShortString(ovr_beaconProxy_getEui48(currProxy), &uuid_str);
				cxa_logger_debug(&bmIn->logger, "updated '%s'  rssi: %d  temp_f: %d  batt:%d%% (%.02fV)  light: %d",
						uuid_str.str, currUpdate->rssi_dBm, (int8_t)((float)currUpdate->currTemp_c * 1.8 + 32.0),
						currUpdate->batt_pcnt100, (float)currUpdate->batt_mv/1000.0, currUpdate->light_255);

				// notify our listeners
				notifyListeners_onUpdate(bmIn, currProxy);

				break;
			}
		}
		if( isKnownProxy ) continue;

		// if we made it here, we have a new proxy...allocate directly in the array
		// so we can maintain the pointer for the listener
		ovr_beaconProxy_t* proxyInArray = (ovr_beaconProxy_t*)cxa_array_append_empty(&bmIn->knownBeacons);
		if( proxyInArray == NULL )
		{
			cxa_logger_warn(&bmIn->logger, "too many beacons in range...dropping");
			return;
		}
		if( !ovr_beaconProxy_init(proxyInArray, currUpdate) )
		{
			cxa_array_remove(&bmIn->knownBeacons, proxyInArray);
			return;
		}

		cxa_eui48_string_t uuid_str;
		cxa_eui48_toString(ovr_beaconProxy_getEui48(proxyInArray), &uuid_str);
		cxa_logger_debug(&bmIn->logger, "new proxy '%s'", uuid_str.str);

		// notify our listeners
		notifyListeners_onFound(bmIn, proxyInArray);
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
			cxa_eui48_string_t uuid_str;
			cxa_eui48_toString(ovr_beaconProxy_getEui48(currProxy), &uuid_str);
			cxa_logger_debug(&bmIn->logger, "lost proxy '%s'", uuid_str.str);

			cxa_array_append(&timedOutProxies, &currProxy);
		}
	}

	// now do the actual removal
	cxa_array_iterate(&timedOutProxies, currProxyPtr, ovr_beaconProxy_t*)
	{
		if( currProxyPtr == NULL ) continue;

		// gotta notify before they're actually removed...otherwise
		// the memory in the array will be freed
		notifyListeners_onLost(bmIn, *currProxyPtr);

		cxa_array_remove(&bmIn->knownBeacons, *currProxyPtr);
	}
}


static void notifyListeners_onFound(ovr_beaconManager_t *const bmIn, ovr_beaconProxy_t *const beaconProxyIn)
{
	cxa_assert(bmIn);
	cxa_assert(beaconProxyIn)

	cxa_array_iterate(&bmIn->listeners, currListener, ovr_beaconManager_listenerEntry_t)
	{
		if( (currListener != NULL) && (currListener->cb_onBeaconFound != NULL) )
		{
			currListener->cb_onBeaconFound(beaconProxyIn, currListener->userVar);
		}
	}
}


static void notifyListeners_onUpdate(ovr_beaconManager_t *const bmIn, ovr_beaconProxy_t *const beaconProxyIn)
{
	cxa_assert(bmIn);
	cxa_assert(beaconProxyIn)

	cxa_array_iterate(&bmIn->listeners, currListener, ovr_beaconManager_listenerEntry_t)
	{
		if( (currListener != NULL) && (currListener->cb_onBeaconUpdate != NULL) )
		{
			currListener->cb_onBeaconUpdate(beaconProxyIn, currListener->userVar);
		}
	}
}


static void notifyListeners_onLost(ovr_beaconManager_t *const bmIn, ovr_beaconProxy_t *const beaconProxyIn)
{
	cxa_assert(bmIn);
	cxa_assert(beaconProxyIn)

	cxa_array_iterate(&bmIn->listeners, currListener, ovr_beaconManager_listenerEntry_t)
	{
		if( (currListener != NULL) && (currListener->cb_onBeaconLost != NULL) )
		{
			currListener->cb_onBeaconLost(beaconProxyIn, currListener->userVar);
		}
	}
}


static void cb_onRunLoopUpdate(void* userVarIn)
{
	ovr_beaconManager_t* bmIn = (ovr_beaconManager_t*)userVarIn;
	cxa_assert(bmIn);

	if( cxa_btle_client_isReady(bmIn->btleClient) &&
		cxa_timeDiff_isElapsed_recurring_ms(&bmIn->td_scanningCheck, SCAN_CHECK_PERIOD_MS) &&
		!cxa_btle_client_isScanning(bmIn->btleClient) )

	{
		cxa_logger_info(&bmIn->logger, "restarting beacon scan");
		cxa_btle_client_startScan_passive(bmIn->btleClient, btleCb_onScanStart, btleCb_onAdvertRx, (void*)bmIn);
	}

	// do the real business
	processRxUpdateFifo(bmIn);
	pruneLostProxies(bmIn);
}


static void btleCb_onReady(cxa_btle_client_t *const btlecIn, void* userVarIn)
{
	ovr_beaconManager_t* bmIn = (ovr_beaconManager_t*)userVarIn;
	cxa_assert(bmIn);

	cxa_timeDiff_setStartTime_now(&bmIn->td_scanningCheck);
	cxa_btle_client_startScan_passive(bmIn->btleClient, btleCb_onScanStart, btleCb_onAdvertRx, (void*)bmIn);
}


static void btleCb_onFailedInit(cxa_btle_client_t *const btlecIn, bool willAutoRetryIn, void* userVarIn)
{
	ovr_beaconManager_t* bmIn = (ovr_beaconManager_t*)userVarIn;
	cxa_assert(bmIn);

	cxa_logger_warn(&bmIn->logger, "BTLE radio failed to boot");
}


static void btleCb_onScanStart(void* userVarIn, bool wasSuccessfulIn)
{
	ovr_beaconManager_t* bmIn = (ovr_beaconManager_t*)userVarIn;
	cxa_assert(bmIn);

	if( !wasSuccessfulIn ) cxa_logger_warn(&bmIn->logger, "failed to start scan");
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
