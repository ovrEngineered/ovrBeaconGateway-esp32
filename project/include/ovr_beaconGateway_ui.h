/**
 * @file
 * @copyright 2017 opencxa.org
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
#ifndef OVR_BEACONGATEWAY_UI_H_
#define OVR_BEACONGATEWAY_UI_H_


// ******** includes ********
#include <cxa_btle_client.h>
#include <cxa_rgbLed.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct ovr_beaconGateway_ui ovr_beaconGateway_ui_t;


/**
 * @private
 */
struct ovr_beaconGateway_ui
{
	cxa_rgbLed_t* led_btleAct;
	cxa_rgbLed_t* led_netAct;

	bool networkError;
};


// ******** global function prototypes ********
void ovr_beaconGateway_ui_init(ovr_beaconGateway_ui_t *const bguiIn, cxa_btle_client_t *const btleClientIn,
		cxa_rgbLed_t *const led_btleActIn, cxa_rgbLed_t *const led_netActIn);


#endif
