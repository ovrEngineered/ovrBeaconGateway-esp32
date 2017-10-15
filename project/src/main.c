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
#include <cxa_nvsManager.h>
#include <cxa_rgbLed.h>
#include <cxa_rgbLed_triLed.h>
#include <cxa_runLoop.h>
#include <cxa_sntpClient.h>
#include <cxa_tempSensor_si7050.h>
#include <cxa_uniqueId.h>

#include <ota_updateClient.h>
#include <ota_logging.h>

#include <ovr_beaconGateway.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


#include "lwip/autoip.h"
#include "esp_event_loop.h"
#include "tcpip_adapter.h"
#include "eth_phy/phy_lan8720.h"
#define DEFAULT_ETHERNET_PHY_CONFIG phy_lan8720_default_ethernet_config

static const char *TAG = "Olimex_ESP32_EVB_REV_B_eth_example";

#define PIN_SMI_MDC		CONFIG_PHY_SMI_MDC_PIN
#define PIN_SMI_MDIO 	CONFIG_PHY_SMI_MDIO_PIN




// ******** local macro definitions ********
#define MQTT_SERVER			"a2vs7wsoau655j.iot.us-east-2.amazonaws.com"
#define MQTT_PORT			8883

#define FW_UUID				"2ba41f5b-9381-4a7c-a97c-c9eed6333f37"


// ******** local type definitions *******


// ******** local function prototypes ********
static void sysInit();
static void thread_network(void *pvParameters);
static void thread_ui(void *pvParameters);
static void thread_bluetooth(void *pvParameters);
static void assertCb();
static void otaUpdate_log(void *userVarIn, int otaLogLevelIn, const char *const tagIn, const char *const fmtIn, va_list argsIn);



static void eth_gpio_config_rmii(void);
static void eth_task(void *pvParameter);



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

static cxa_esp32_gpio_t gpio_variant_external;
static cxa_esp32_gpio_t gpio_variant_internalHighPower;

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
    sysInit();
}


// ******** local function implementations ********
static void sysInit()
{
	// setup our debug serial port
	cxa_esp32_usart_init(&usart_debug, UART_NUM_0, 115200, GPIO_NUM_1, GPIO_NUM_3, false);
	cxa_assert_setIoStream(cxa_usart_getIoStream(&usart_debug.super));
	cxa_assert_setAssertCb(assertCb);

	// setup our NVS manager
	cxa_nvsManager_init();

	// setup our timebase
	cxa_esp32_timeBase_init();

	// setup our console and logger
	cxa_console_init("ovrBeacon Gateway", cxa_usart_getIoStream(&usart_debug.super), OVR_GW_THREADID_UI);
	cxa_logger_setGlobalIoStream(cxa_usart_getIoStream(&usart_debug.super));

	// setup our networking
	cxa_network_wifiManager_init(OVR_GW_THREADID_NETWORK);
//	cxa_sntpClient_init();
//	cxa_mqtt_connManager_init(MQTT_SERVER, MQTT_PORT,
//							  OVR_GW_THREADID_NETWORK);
//	cxa_mqtt_rpc_node_root_init(&rpcNode_root, cxa_mqtt_connManager_getMqttClient(), true, cxa_uniqueId_getHexString());
//
//	// setup our application-specific peripherals
//	cxa_esp32_gpio_init_output(&gpio_led_btleAct_r, GPIO_NUM_32, CXA_GPIO_POLARITY_INVERTED, 0);
//	cxa_esp32_gpio_init_output(&gpio_led_btleAct_g, GPIO_NUM_23, CXA_GPIO_POLARITY_INVERTED, 0);
//	cxa_esp32_gpio_init_output(&gpio_led_btleAct_b, GPIO_NUM_22, CXA_GPIO_POLARITY_INVERTED, 0);
//	cxa_led_gpio_init(&led_btleAct_r, &gpio_led_btleAct_r.super, true);
//	cxa_led_gpio_init(&led_btleAct_g, &gpio_led_btleAct_g.super, true);
//	cxa_led_gpio_init(&led_btleAct_b, &gpio_led_btleAct_b.super, true);
//	cxa_rgbLed_triLed_init(&led_btleAct, &led_btleAct_r.super, &led_btleAct_g.super, &led_btleAct_b.super, OVR_GW_THREADID_UI);
//
//	cxa_esp32_gpio_init_output(&gpio_led_netAct_r, GPIO_NUM_26, CXA_GPIO_POLARITY_INVERTED, 0);
//	cxa_esp32_gpio_init_output(&gpio_led_netAct_g, GPIO_NUM_33, CXA_GPIO_POLARITY_INVERTED, 0);
//	cxa_esp32_gpio_init_output(&gpio_led_netAct_b, GPIO_NUM_25, CXA_GPIO_POLARITY_INVERTED, 0);
//	cxa_led_gpio_init(&led_netAct_r, &gpio_led_netAct_r.super, true);
//	cxa_led_gpio_init(&led_netAct_g, &gpio_led_netAct_g.super, true);
//	cxa_led_gpio_init(&led_netAct_b, &gpio_led_netAct_b.super, true);
//	cxa_rgbLed_triLed_init(&led_netAct, &led_netAct_r.super, &led_netAct_g.super, &led_netAct_b.super, OVR_GW_THREADID_UI);
//
//	cxa_esp32_gpio_init_input(&gpio_swProvision, GPIO_NUM_27, CXA_GPIO_POLARITY_INVERTED);
//	cxa_esp32_gpio_init_output(&gpio_btleReset, GPIO_NUM_4, CXA_GPIO_POLARITY_INVERTED, 0);
//
//	cxa_esp32_gpio_init_input(&gpio_variant_internalHighPower, GPIO_NUM_18, CXA_GPIO_POLARITY_INVERTED);
//	cxa_esp32_gpio_setPullMode(&gpio_variant_internalHighPower, GPIO_PULLUP_ONLY);
//	cxa_esp32_gpio_init_input(&gpio_variant_external, GPIO_NUM_12, CXA_GPIO_POLARITY_INVERTED);
//	cxa_esp32_gpio_setPullMode(&gpio_variant_external, GPIO_PULLUP_ONLY);
//
//	cxa_esp32_usart_init(&usart_btle, UART_NUM_1, 115200, GPIO_NUM_17, GPIO_NUM_16, false);
//	cxa_blueGiga_btle_client_init(&btleClient, cxa_usart_getIoStream(&usart_btle.super),
//								  &gpio_btleReset.super, OVR_GW_THREADID_BLUETOOTH);
//
//	cxa_lightSensor_ltr329_init(&lightSensor, cxa_blueGiga_btle_client_getI2cMaster(&btleClient), OVR_GW_THREADID_BLUETOOTH);
//	cxa_tempSensor_si7050_init(&tempSensor, cxa_blueGiga_btle_client_getI2cMaster(&btleClient));
//	ovr_beaconGateway_init(&beaconGateway, &btleClient.super, &gpio_swProvision.super,
//						   &gpio_variant_internalHighPower.super, &gpio_variant_external.super,
//						   &led_btleAct.super, &led_netAct.super,
//						   &lightSensor.super, &tempSensor.super, &rpcNode_root.super);
//
//	// initialize our otaUpdate client
//	ota_updateClient_init(FW_UUID, cxa_uniqueId_getHexString());
//	ota_updateClient_setLogFunction(otaUpdate_log, NULL);

	// schedule our user task for execution
	xTaskCreate(thread_network, (const char * const)"net", 4096, NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(thread_ui, (const char * const)"ui", 2048, NULL, tskIDLE_PRIORITY, NULL);
//	xTaskCreate(thread_bluetooth, (const char * const)"bt", 4096, NULL, tskIDLE_PRIORITY, NULL);



	esp_err_t ret = ESP_OK;
	tcpip_adapter_init();
	esp_event_loop_init(NULL, NULL);

	eth_config_t config = DEFAULT_ETHERNET_PHY_CONFIG;
	config.phy_addr = CONFIG_PHY_ADDRESS;
	config.gpio_config = eth_gpio_config_rmii;
	config.tcpip_input = tcpip_adapter_eth_input;

	ret = esp_eth_init(&config);

	if(ret == ESP_OK)
	{
		esp_eth_enable();
		xTaskCreate(eth_task, "eth_task", 2048, NULL, (tskIDLE_PRIORITY + 2), NULL);
	}
}


static void thread_network(void *pvParameters)
{
	cxa_network_wifiManager_start();

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


static void assertCb()
{
	vTaskSuspendAll();
	ovr_beaconGateway_onAssert(&beaconGateway);
}


static void otaUpdate_log(void *userVarIn, int otaLogLevelIn, const char *const tagIn, const char *const fmtIn, va_list argsIn)
{
	static cxa_logger_t logger;
	static bool isInit = false;
	if( !isInit )
	{
		cxa_logger_init(&logger, "otaUpdate");
		isInit = true;
	}

	int logLevel = CXA_LOG_LEVEL_NONE;
	switch( otaLogLevelIn )
	{
		case OTA_LOG_LEVEL_ERROR:
			logLevel = CXA_LOG_LEVEL_ERROR;
			break;

		case OTA_LOG_LEVEL_WARN:
			logLevel = CXA_LOG_LEVEL_WARN;
			break;

		case OTA_LOG_LEVEL_INFO:
			logLevel = CXA_LOG_LEVEL_INFO;
			break;

		case OTA_LOG_LEVEL_DEBUG:
			logLevel = CXA_LOG_LEVEL_DEBUG;
			break;
	}

	cxa_logger_log_varArgs(&logger, logLevel, fmtIn, argsIn);
}









static void eth_gpio_config_rmii(void)
{
    // RMII data pins are fixed:
    // TXD0 = GPIO19
    // TXD1 = GPIO22
    // TX_EN = GPIO21
    // RXD0 = GPIO25
    // RXD1 = GPIO26
    // CLK == GPIO0
    phy_rmii_configure_data_interface_pins();
    // MDC is GPIO 23, MDIO is GPIO 18
    phy_rmii_smi_configure_pins(PIN_SMI_MDC, PIN_SMI_MDIO);
}


static void eth_task(void *pvParameter)
{
    tcpip_adapter_ip_info_t ip;
    memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
    vTaskDelay(2000 / portTICK_PERIOD_MS);

//    tcpip_adapter_dhcps_stop(ESP_IF_ETH);
//
//    tcpip_adapter_ip_info_t ip_info;
//    IP4_ADDR(&ip_info.ip, 192, 168, 1, 67);
//    IP4_ADDR(&ip_info.gw, 192, 168, 1, 1);
//    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
//    tcpip_adapter_set_ip_info(ESP_IF_ETH, &ip_info);

    while (1)
    {
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        if (tcpip_adapter_get_ip_info(ESP_IF_ETH, &ip) == 0) {
        		cxa_logger_debug(cxa_logger_getSysLog(), "~~~~~~~~~~~");
        		cxa_logger_debug(cxa_logger_getSysLog(), "ETHIP:"IPSTR, IP2STR(&ip.ip));
        		cxa_logger_debug(cxa_logger_getSysLog(), "ETHPMASK:"IPSTR, IP2STR(&ip.netmask));
        		cxa_logger_debug(cxa_logger_getSysLog(), "ETHPGW:"IPSTR, IP2STR(&ip.gw));
        		cxa_logger_debug(cxa_logger_getSysLog(), "~~~~~~~~~~~");
        }
    }
}
