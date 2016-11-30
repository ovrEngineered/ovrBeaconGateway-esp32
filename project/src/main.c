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
#include <cxa_esp32_gpio.h>
#include <cxa_esp32_timeBase.h>
#include <cxa_esp32_usart.h>
#include <cxa_network_wifiManager.h>
#include <cxa_runLoop.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions *******


// ******** local function prototypes ********
static void sysInit();
static void userTask(void *pvParameters);


// ******** local variable declarations ********
static cxa_esp32_gpio_t gpio_led;
static cxa_esp32_usart_t usart_debug;


// ******** global function implementations ********
void app_main(void)
{
    nvs_flash_init();


//    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
//    wifi_config_t sta_config = {
//        .sta = {
//            .ssid = "access_point_name",
//            .password = "password",
//            .bssid_set = false
//        }
//    };
//    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
//    ESP_ERROR_CHECK( esp_wifi_start() );
//    ESP_ERROR_CHECK( esp_wifi_connect() );

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
	cxa_console_init("Haven ECB", cxa_usart_getIoStream(&usart_debug.super));
	cxa_logger_setGlobalIoStream(cxa_usart_getIoStream(&usart_debug.super));

	// setup our application-specific peripherals
	cxa_network_wifiManager_init();
	cxa_esp32_gpio_init_output(&gpio_led, GPIO_NUM_4, CXA_GPIO_POLARITY_NONINVERTED, 0);

	xTaskCreate(userTask, (const char * const)"usrTask", 2048, NULL, tskIDLE_PRIORITY, NULL);
}


static void userTask(void *pvParameters)
{
	cxa_network_wifiManager_start();

	// does not return
	cxa_runLoop_execute();
}
