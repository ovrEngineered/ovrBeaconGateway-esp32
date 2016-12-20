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

#include <cxa_assert.h>
#include <cxa_console.h>
#include <cxa_delay.h>
#include <cxa_esp32_btle_client.h>
#include <cxa_esp32_gpio.h>
#include <cxa_esp32_timeBase.h>
#include <cxa_esp32_usart.h>
#include <cxa_network_wifiManager.h>
#include <cxa_runLoop.h>
#include <cxa_uniqueId.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>

#include "aws_certs.h"
#include <cxa_mqtt_connectionManager.h>
#include <cxa_mqtt_rpc_node_root.h>


// ******** local macro definitions ********
#define MQTT_SERVER 		"A218ORPUJ66O9J.iot.us-east-1.amazonaws.com"
#define MQTT_PORT 			8883


// ******** local type definitions *******


// ******** local function prototypes ********
static void sysInit();
static void userTask(void *pvParameters);

static cxa_mqtt_rpc_methodRetVal_t mqttRpcCb_onSetLed(cxa_mqtt_rpc_node_t *const nodeIn,
		cxa_linkedField_t *const paramsIn, cxa_linkedField_t *const returnParamsOut,
		void* userVarIn);

static void consoleCb_rpcNotify(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);


// ******** local variable declarations ********
static cxa_esp32_gpio_t gpio_led;
static cxa_esp32_usart_t usart_debug;

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
	cxa_esp32_usart_init_noHH(&usart_debug, UART_NUM_0, 115200);
	cxa_assert_setIoStream(cxa_usart_getIoStream(&usart_debug.super));

	// setup our timebase
	cxa_esp32_timeBase_init();

	// setup our console and logger
	cxa_console_init("AWS IoT Tester", cxa_usart_getIoStream(&usart_debug.super));
	cxa_logger_setGlobalIoStream(cxa_usart_getIoStream(&usart_debug.super));

	// setup our application-specific peripherals
	cxa_network_wifiManager_init();
	cxa_esp32_gpio_init_output(&gpio_led, GPIO_NUM_4, CXA_GPIO_POLARITY_NONINVERTED, 0);

	cxa_mqtt_connManager_init_clientCert(NULL,
										 MQTT_SERVER, MQTT_PORT,
										 tls_server_root_cert, sizeof(tls_server_root_cert),
										 tls_client_cert, sizeof(tls_client_cert),
										 tls_client_key_private, sizeof(tls_client_key_private));
	cxa_mqtt_rpc_node_root_init(&rpcNode_root, cxa_mqtt_connManager_getMqttClient(), true, cxa_uniqueId_getHexString());

	// XXX/->setLed/0000
	cxa_mqtt_rpc_node_addMethod(&rpcNode_root.super, "setLed", mqttRpcCb_onSetLed, NULL);
	cxa_console_addCommand("rpc_notify", "sends a notification to the server", NULL, 0, consoleCb_rpcNotify, NULL);

	// schedule our user task for execution
	xTaskCreate(userTask, (const char * const)"usrTask", 4096, NULL, tskIDLE_PRIORITY, NULL);
}


static void userTask(void *pvParameters)
{
	cxa_network_wifiManager_startNormal();

	// does not return
	cxa_runLoop_execute();
}


static cxa_mqtt_rpc_methodRetVal_t mqttRpcCb_onSetLed(cxa_mqtt_rpc_node_t *const nodeIn,
		cxa_linkedField_t *const paramsIn, cxa_linkedField_t *const returnParamsOut,
		void* userVarIn)
{
	if( cxa_linkedField_getSize_bytes(paramsIn) != 1 ) return CXA_MQTT_RPC_METHODRETVAL_FAIL_INVALIDPARAMS;

	uint8_t ledVal = 0;
	cxa_linkedField_get_uint8(paramsIn, 0, ledVal);
	if( ledVal == '0' ) ledVal = 0;

	cxa_gpio_setValue(&gpio_led.super, ledVal);

	return CXA_MQTT_RPC_METHODRETVAL_SUCCESS;
}


static void consoleCb_rpcNotify(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	uint8_t ledState = cxa_gpio_getValue(&gpio_led.super);
	cxa_mqtt_rpc_node_publishNotification(&rpcNode_root.super, "ledStatus", CXA_MQTT_QOS_ATMOST_ONCE, &ledState, sizeof(ledState));
}
