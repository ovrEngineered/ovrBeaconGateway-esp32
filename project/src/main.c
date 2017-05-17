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


// ******** includes ********
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "nvs_flash.h"

#include "aws_certs.h"

#include <cxa_assert.h>
#include <cxa_blueGiga_btle_client.h>
#include <cxa_console.h>
#include <cxa_delay.h>
#include <cxa_esp32_btle_client.h>
#include <cxa_esp32_gpio.h>
#include <cxa_esp32_timeBase.h>
#include <cxa_esp32_usart.h>
#include <cxa_ioStream_bridge.h>
#include <cxa_led_gpio.h>
#include <cxa_lightSensor_ltr329.h>
#include <cxa_mqtt_connectionManager.h>
#include <cxa_mqtt_rpc_node_root.h>
#include <cxa_network_wifiManager.h>
#include <cxa_rgbLed.h>
#include <cxa_rgbLed_triLed.h>
#include <cxa_runLoop.h>
#include <cxa_sntpClient.h>
#include <cxa_tempSensor_si7050.h>
#include <cxa_uniqueId.h>

#include <ovr_beaconGateway.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define MQTT_SERVER 		"A218ORPUJ66O9J.iot.us-east-1.amazonaws.com"
#define MQTT_PORT 			8883


// ******** local type definitions *******


// ******** local function prototypes ********
static void sysInit();
static void thread_network(void *pvParameters);
static void thread_ui(void *pvParameters);
static void thread_bluetooth(void *pvParameters);


// ******** local variable declarations ********
static cxa_esp32_gpio_t gpio_led_btleAct_r;
static cxa_esp32_gpio_t gpio_led_btleAct_g;
static cxa_esp32_gpio_t gpio_led_btleAct_b;
static cxa_led_gpio_t led_btleAct_r;
static cxa_led_gpio_t led_btleAct_g;
static cxa_led_gpio_t led_btleAct_b;
static cxa_rgbLed_triLed_t led_btleAct;

static cxa_esp32_gpio_t gpio_led_netAct_r;
static cxa_esp32_gpio_t gpio_led_netAct_g;
static cxa_esp32_gpio_t gpio_led_netAct_b;
static cxa_led_gpio_t led_netAct_r;
static cxa_led_gpio_t led_netAct_g;
static cxa_led_gpio_t led_netAct_b;
static cxa_rgbLed_triLed_t led_netAct;

static cxa_esp32_gpio_t gpio_btleReset;
static cxa_esp32_gpio_t gpio_swProvision;

static cxa_esp32_usart_t usart_debug;

static cxa_esp32_usart_t usart_btle;
static cxa_blueGiga_btle_client_t btleClient;

static cxa_lightSensor_ltr329_t lightSensor;
static cxa_tempSensor_si7050_t tempSensor;

static ovr_beaconGateway_t beaconGateway;

static cxa_mqtt_rpc_node_root_t rpcNode_root;


// ******** global function implementations ********
void app_main(void)
{
    nvs_flash_init();

    sysInit();
}


// ******** local function implementations ********
static void sysInit()
{
	// setup our debug serial console
	cxa_esp32_usart_init(&usart_debug, UART_NUM_0, 115200, GPIO_NUM_1, GPIO_NUM_3, false);
	cxa_assert_setIoStream(cxa_usart_getIoStream(&usart_debug.super));

	// setup our timebase
	cxa_esp32_timeBase_init();

	// setup our console and logger
	cxa_console_init("ovrBeacon Gateway", cxa_usart_getIoStream(&usart_debug.super), OVR_GW_THREADID_UI);
	cxa_logger_setGlobalIoStream(cxa_usart_getIoStream(&usart_debug.super));

	// setup our networking
	cxa_network_wifiManager_init(OVR_GW_THREADID_NETWORK);
	cxa_sntpClient_init();
	cxa_mqtt_connManager_init_clientCert(NULL,
										 MQTT_SERVER, MQTT_PORT,
										 tls_server_root_cert, sizeof(tls_server_root_cert),
										 tls_client_cert, sizeof(tls_client_cert),
										 tls_client_key_private, sizeof(tls_client_key_private),
										 OVR_GW_THREADID_NETWORK);
	cxa_mqtt_rpc_node_root_init(&rpcNode_root, cxa_mqtt_connManager_getMqttClient(), true, cxa_uniqueId_getHexString());

	// setup our application-specific peripherals
	cxa_esp32_gpio_init_output(&gpio_led_btleAct_r, GPIO_NUM_32, CXA_GPIO_POLARITY_INVERTED, 0);
	cxa_esp32_gpio_init_output(&gpio_led_btleAct_g, GPIO_NUM_23, CXA_GPIO_POLARITY_INVERTED, 0);
	cxa_esp32_gpio_init_output(&gpio_led_btleAct_b, GPIO_NUM_22, CXA_GPIO_POLARITY_INVERTED, 0);
	cxa_led_gpio_init(&led_btleAct_r, &gpio_led_btleAct_r.super, true);
	cxa_led_gpio_init(&led_btleAct_g, &gpio_led_btleAct_g.super, true);
	cxa_led_gpio_init(&led_btleAct_b, &gpio_led_btleAct_b.super, true);
	cxa_rgbLed_triLed_init(&led_btleAct, &led_btleAct_r.super, &led_btleAct_g.super, &led_btleAct_b.super, OVR_GW_THREADID_UI);

	cxa_esp32_gpio_init_output(&gpio_led_netAct_r, GPIO_NUM_26, CXA_GPIO_POLARITY_INVERTED, 0);
	cxa_esp32_gpio_init_output(&gpio_led_netAct_g, GPIO_NUM_33, CXA_GPIO_POLARITY_INVERTED, 0);
	cxa_esp32_gpio_init_output(&gpio_led_netAct_b, GPIO_NUM_25, CXA_GPIO_POLARITY_INVERTED, 0);
	cxa_led_gpio_init(&led_netAct_r, &gpio_led_netAct_r.super, true);
	cxa_led_gpio_init(&led_netAct_g, &gpio_led_netAct_g.super, true);
	cxa_led_gpio_init(&led_netAct_b, &gpio_led_netAct_b.super, true);
	cxa_rgbLed_triLed_init(&led_netAct, &led_netAct_r.super, &led_netAct_g.super, &led_netAct_b.super, OVR_GW_THREADID_UI);

	cxa_esp32_gpio_init_input(&gpio_swProvision, GPIO_NUM_27, CXA_GPIO_POLARITY_INVERTED);
	cxa_esp32_gpio_init_output(&gpio_btleReset, GPIO_NUM_4, CXA_GPIO_POLARITY_INVERTED, 0);

	cxa_esp32_usart_init(&usart_btle, UART_NUM_1, 115200, GPIO_NUM_17, GPIO_NUM_16, false);
	cxa_blueGiga_btle_client_init(&btleClient, cxa_usart_getIoStream(&usart_btle.super),
								  &gpio_btleReset.super, OVR_GW_THREADID_BLUETOOTH);

	cxa_lightSensor_ltr329_init(&lightSensor, cxa_blueGiga_btle_client_getI2cMaster(&btleClient), OVR_GW_THREADID_BLUETOOTH);
	cxa_tempSensor_si7050_init(&tempSensor, cxa_blueGiga_btle_client_getI2cMaster(&btleClient));
	ovr_beaconGateway_init(&beaconGateway, &btleClient.super, &gpio_swProvision.super,
						   &led_btleAct.super, &led_netAct.super,
						   &lightSensor.super, &tempSensor.super, &rpcNode_root.super);

	// schedule our user task for execution
	xTaskCreate(thread_network, (const char * const)"net", 4096, NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(thread_ui, (const char * const)"ui", 1024, NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(thread_bluetooth, (const char * const)"bt", 4096, NULL, tskIDLE_PRIORITY, NULL);
}


static void thread_network(void *pvParameters)
{
	cxa_network_wifiManager_startNormal();

	// does not return
	cxa_runLoop_execute(OVR_GW_THREADID_NETWORK);
}


static void thread_ui(void *pvParameters)
{
	// does not return
	cxa_runLoop_execute(OVR_GW_THREADID_UI);
}


static void thread_bluetooth(void *pvParameters)
{
	// does not return
	cxa_runLoop_execute(OVR_GW_THREADID_BLUETOOTH);
}
