/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"

#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>

#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"

#include <esp_idf_version.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0)
// Features supported in 4.1+
#define ESP_NETIF_SUPPORTED
#endif

static const char *TAG = "publish";

#define EXAMPLE_WIFI_SSID "SpectrumSetup-15"
#define EXAMPLE_WIFI_PASS "purplegate367"
#define CONFIG_EXAMPLE_FILESYSTEM_CERTS 1
static const char * CONFIG_AWS_EXAMPLE_CLIENT_ID = "iotconsole-2c0886a8-1de3-4346-a42c-2837d7924a2d";

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

const int CONNECTED_BIT = BIT0;

IoT_Error_t rc = FAILURE;

const char *TOPIC = "sensor/car_count";

AWS_IoT_Client client;

/* CA Root certificate, device ("Thing") certificate and device
 * ("Thing") key.

   Example can be configured one of two ways:

   "Embedded Certs" are loaded from files in "certs/" and embedded into the app binary.

   "Filesystem Certs" are loaded from the filesystem (SD card, etc.)

   See example README for more details.
*/
#if defined(CONFIG_EXAMPLE_EMBEDDED_CERTS)

extern const uint8_t aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");
extern const uint8_t aws_root_ca_pem_end[] asm("_binary_aws_root_ca_pem_end");
extern const uint8_t certificate_pem_crt_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t certificate_pem_crt_end[] asm("_binary_certificate_pem_crt_end");
extern const uint8_t private_pem_key_start[] asm("_binary_private_pem_key_start");
extern const uint8_t private_pem_key_end[] asm("_binary_private_pem_key_end");

#elif defined(CONFIG_EXAMPLE_FILESYSTEM_CERTS)
static const char * DEVICE_CERTIFICATE_PATH = 
"-----BEGIN CERTIFICATE-----\n\
MIIDWTCCAkGgAwIBAgIUKGG29H6JgTafnh9VaP/n/ouFRyQwDQYJKoZIhvcNAQEL\n\
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n\
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTIzMTAyMTA1MjA1\n\
MVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n\
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALhdU42wEMwWzlMdo25V\n\
wb5NMQyh4+8DESwLv9x2qFNzzIUTFaCGKVjQKbfkm0Ypb1r0A5BnFq8dzeshdLNZ\n\
W5aiKSiyZFIAcDbCfio5ZDYk5LpfCAxRNlbmHAlpd1/bIbG2RpUY0QOP5Qu+FgB5\n\
grftu0+os1wloQUGthBfhPc1+z3JMBpdQme8uFI6WxXpgEGpEn5ziaSriCbMhrse\n\
Y5gg2z52GrAfCNkNPRhxtRtsm88QM1r1GZSJUX2bWlpoZowOqSaR1JpGXD9RKaZ6\n\
EGHKQFrYqCMB3npo2y3mmWuMR/lPSFb6k+frJjJNckP2IAx3E2NpBaaDghvoO5js\n\
epMCAwEAAaNgMF4wHwYDVR0jBBgwFoAUaKExchpRF4q0rLt0eHPtWblZBsgwHQYD\n\
VR0OBBYEFBrqIU0iix/juJGE238M8BaIeyjIMAwGA1UdEwEB/wQCMAAwDgYDVR0P\n\
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQBEpi7KgjFTjiZXWC2/rzo/9kCL\n\
Y96lcYf9fo5hzdJqEKiD/YVgnp+/6/ZDCwA3/6cxuPDy7XFqGSQW+v3gAcZnhH2t\n\
bAaswd7D3JVrd6feRqcwkzqu2i0FEEL9AS2VpYuZ6QDjzhXoRnWJgxiXVBb3kmWM\n\
cemvGW9eRaUZScjqIgVdnKa6zKjXz1Ve9ipn80ByQ4C31ZwD4Zyfd9tyds/ICfhH\n\
D7BadO4k7CTR4mwbhcnkssawAWN9DHkPfeEdJfke7bT+/UPMjK4mEGP3FgrVpv23\n\
BWQ9XzNr8wf/oC8v/ch0VhOIZxSLEJFpoWdhemlL4j3B8xPfmkH5WymEk6jX\n\
-----END CERTIFICATE-----\n";

static const char * DEVICE_PRIVATE_KEY_PATH = 
"-----BEGIN RSA PRIVATE KEY-----\n\
MIIEpAIBAAKCAQEAuF1TjbAQzBbOUx2jblXBvk0xDKHj7wMRLAu/3HaoU3PMhRMV\n\
oIYpWNApt+SbRilvWvQDkGcWrx3N6yF0s1lblqIpKLJkUgBwNsJ+KjlkNiTkul8I\n\
DFE2VuYcCWl3X9shsbZGlRjRA4/lC74WAHmCt+27T6izXCWhBQa2EF+E9zX7Pckw\n\
Gl1CZ7y4UjpbFemAQakSfnOJpKuIJsyGux5jmCDbPnYasB8I2Q09GHG1G2ybzxAz\n\
WvUZlIlRfZtaWmhmjA6pJpHUmkZcP1EppnoQYcpAWtioIwHeemjbLeaZa4xH+U9I\n\
VvqT5+smMk1yQ/YgDHcTY2kFpoOCG+g7mOx6kwIDAQABAoIBAE4R9xfAvtwtrCE2\n\
I8bNOcXEGkknJMZJnjPwpcZN2Om915Vih336Uffx7jiY7cfANA9n1TqI7OoqzNAM\n\
0sY/yLD46kT5hdHTrbECmzp2IyFqbhEdvOK6YTBbMPQrXQmSWapPbYQlhhzkCjH2\n\
xIpe6iPI39DTt2AF7zGZre8xA7VKxerpmerwJ6YiqToXcAx7y4aHygyBLDb7Wybz\n\
yc59xCNM8caOof90wgupBhiajLvZu/K7uj7PyY71XJd8xF3CqShCk6ZUb9Nbd4tP\n\
29aj8A/nUdvxQPGlKTVS4v97JD6KLAjSF+cSxpHfHhUzoKsvK1P+1UZOlkN309mz\n\
mcNtPkECgYEA7VKDMsDBBh9nJy3ZjOiorX8YvUm7WBHGdcEYP27RxSO0FBxMzZ23\n\
uZjmSNEk+c4ytMX/zQYlZ7+CfldI8tATL0vcRmY0VdS/RDK+SqUFAmYxkOuOgSa1\n\
kYqVP7h8ER+25O4+omQgAxSoIZD3+DC6JcyTJc7xP1Zr+c2sHQyd2M0CgYEAxt/W\n\
4aJtpWJv2CoN6YTjUrOgV2FWL26qKZ8zkghpFITyAeq8hRneVFa0oJQKyDsWHPB+\n\
KbiQvirvi9FLuitKvygts1QtxBGB+af+ny+CIOm+TiQP9KEtR5m9+19LWx4FQc6k\n\
mn/aqUhl7JtETUqoLUmcjh0adCiN+2n8xR2fIN8CgYEAiSJ66aL0ZwNSahNWeNQg\n\
VFDzDL5EYXm0AmtYBZ+V26Lr9gr8XnxapAa0WzNveGIsbsVTMTfx+WdykmsBnj2a\n\
OGRPnuaEK2zKMTBZQWzl3iMLVUCokfy6QqWf5LhICOUYnGUHEHNnBCC1nt/USjht\n\
+FWkWd6hDQZ1Ul4ErKyTsakCgYEAkN2ezo+eJTnWVPUVW0Rkvup/4wbRXA3VwCnq\n\
y/Z6bOsGyqSuHJqiXHcobkxIqmB4FC6PCF8ceJoYEpYr/nnooeRnndi02b0TG8Pm\n\
8xgNR2JdMNiOEtmRfTWdsU1SPBZbJ/uZ5b06j9NgA4F6uH6okQytEMxcZ77e5gPn\n\
j8KrFqsCgYAqjl2FzxE7vuDpB3PqN/0OymNsoufjKqxs7FZ1mCBRomxxNDVP+QXd\n\
T2PGEVDS8vnB6yilEPRTI7pCbKC3G9Gje7lGloAlMOYpcFyt32DqpPJ2xUVVkU+c\n\
oCtbGftoiYo5jZ4fK6uRB23MvzA2GKi5qUAWg7JDb0kSElnAhXesVQ==\n\
-----END RSA PRIVATE KEY-----\n";

static const char * ROOT_CA_PATH = 
"-----BEGIN CERTIFICATE-----\n\
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n\
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n\
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n\
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n\
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n\
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n\
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n\
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n\
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n\
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n\
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n\
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n\
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n\
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n\
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n\
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n\
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n\
rqXRfboQnoZsG4q5WTP468SQvvG5\n\
-----END CERTIFICATE-----\n";
#else
#error "Invalid method for loading certs"
#endif

/**
 * @brief Default MQTT HOST URL is pulled from the aws_iot_config.h
 */
char HostAddress[255] = AWS_IOT_MQTT_HOST;
uint32_t port = AWS_IOT_MQTT_PORT;


static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    }
}

void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                    IoT_Publish_Message_Params *params, void *pData) {
    ESP_LOGI(TAG, "Subscribe callback");
    ESP_LOGI(TAG, "%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *)params->payload);
}

void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data) {
    ESP_LOGW(TAG, "MQTT Disconnect");
    IoT_Error_t rc = FAILURE;

    if(NULL == pClient) {
        return;
    }

    if(aws_iot_is_autoreconnect_enabled(pClient)) {
        ESP_LOGI(TAG, "Auto Reconnect is enabled, Reconnecting attempt will start now");
    } else {
        ESP_LOGW(TAG, "Auto Reconnect not enabled. Starting manual reconnect...");
        rc = aws_iot_mqtt_attempt_reconnect(pClient);
        if(NETWORK_RECONNECTED == rc) {
            ESP_LOGW(TAG, "Manual Reconnect Successful");
        } else {
            ESP_LOGW(TAG, "Manual Reconnect Failed - %d", rc);
        }
    }
}

void aws_iot_task(void *param) {
    char cPayload[100];

    int i = 0;

    const int TOPIC_LEN = strlen(TOPIC);

    IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
    IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

    IoT_Publish_Message_Params paramsQOS0;
    IoT_Publish_Message_Params paramsQOS1;

    ESP_LOGI(TAG, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

    mqttInitParams.enableAutoReconnect = false; // We enable this later below
    mqttInitParams.pHostURL = HostAddress;
    mqttInitParams.port = port;

#if defined(CONFIG_EXAMPLE_EMBEDDED_CERTS)
    mqttInitParams.pRootCALocation = (const char *)aws_root_ca_pem_start;
    mqttInitParams.pDeviceCertLocation = (const char *)certificate_pem_crt_start;
    mqttInitParams.pDevicePrivateKeyLocation = (const char *)private_pem_key_start;

#elif defined(CONFIG_EXAMPLE_FILESYSTEM_CERTS)
    mqttInitParams.pRootCALocation = ROOT_CA_PATH;
    mqttInitParams.pDeviceCertLocation = DEVICE_CERTIFICATE_PATH;
    mqttInitParams.pDevicePrivateKeyLocation = DEVICE_PRIVATE_KEY_PATH;
#endif

    mqttInitParams.mqttCommandTimeout_ms = 20000;
    mqttInitParams.tlsHandshakeTimeout_ms = 5000;
    mqttInitParams.isSSLHostnameVerify = true;
    mqttInitParams.disconnectHandler = disconnectCallbackHandler;
    mqttInitParams.disconnectHandlerData = NULL;

#ifdef CONFIG_EXAMPLE_SDCARD_CERTS
    ESP_LOGI(TAG, "Mounting SD card...");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 3,
    };
    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card VFAT filesystem. Error: %s", esp_err_to_name(ret));
        abort();
    }
#endif

    rc = aws_iot_mqtt_init(&client, &mqttInitParams);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "aws_iot_mqtt_init returned error : %d ", rc);
        abort();
    }

    /* Wait for WiFI to show as connected */
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, portMAX_DELAY);

    connectParams.keepAliveIntervalInSec = 10;
    connectParams.isCleanSession = true;
    connectParams.MQTTVersion = MQTT_3_1_1;
    /* Client ID is set in the menuconfig of the example */
    connectParams.pClientID = CONFIG_AWS_EXAMPLE_CLIENT_ID;
    connectParams.clientIDLen = (uint16_t) strlen(CONFIG_AWS_EXAMPLE_CLIENT_ID);
    connectParams.isWillMsgPresent = false;

    ESP_LOGI(TAG, "Connecting to AWS...");
    do {
        rc = aws_iot_mqtt_connect(&client, &connectParams);
        if(SUCCESS != rc) {
            ESP_LOGE(TAG, "Error(%d) connecting to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    } while(SUCCESS != rc);

    /*
     * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
     *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
     *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
     */
    rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "Unable to set Auto Reconnect to true - %d", rc);
        abort();
    }



    ESP_LOGI(TAG, "Subscribing...");
    rc = aws_iot_mqtt_subscribe(&client, TOPIC, TOPIC_LEN, QOS0, iot_subscribe_callback_handler, NULL);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "Error subscribing : %d ", rc);
        abort();
    }

    sprintf(cPayload, "%s : %d ", "hello from SDK", i);

    // paramsQOS0.qos = QOS0;
    // paramsQOS0.payload = (void *) cPayload;
    // paramsQOS0.isRetained = 0;

    // paramsQOS1.qos = QOS1;
    // paramsQOS1.payload = (void *) cPayload;
    // paramsQOS1.isRetained = 0;

    while((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc)) {

        //Max time the yield function will wait for read messages
        rc = aws_iot_mqtt_yield(&client, 100);
        if(NETWORK_ATTEMPTING_RECONNECT == rc) {
            // If the client is attempting to reconnect we will skip the rest of the loop.
            continue;
        }

        //ESP_LOGI(TAG, "Stack remaining for task '%s' is %d bytes", pcTaskGetName(NULL), uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        //sprintf(cPayload, "%s : %d ", "hello from ESP32 (QOS0)", i++);
        //paramsQOS0.payloadLen = strlen(cPayload);
        //rc = aws_iot_mqtt_publish(&client, TOPIC, TOPIC_LEN, &paramsQOS0);

        //sprintf(cPayload, "%s : %d ", "hello from ESP32 (QOS1)", i++);
        //paramsQOS1.payloadLen = strlen(cPayload);
        //rc = aws_iot_mqtt_publish(&client, TOPIC, TOPIC_LEN, &paramsQOS1);
        // if (rc == MQTT_REQUEST_TIMEOUT_ERROR) {
        //     ESP_LOGW(TAG, "QOS1 publish ack not received.");
        //     rc = SUCCESS;
        // }
    }

    ESP_LOGE(TAG, "An error occurred in the main loop.");
    abort();
}


static void initialise_wifi(void)
{
    /* Initialize TCP/IP */
#ifdef ESP_NETIF_SUPPORTED
    esp_netif_init();
#else
    tcpip_adapter_init();
#endif
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    /* Initialize Wi-Fi including netif with default config */
#ifdef ESP_NETIF_SUPPORTED
    esp_netif_create_default_wifi_sta();
#endif

    wifi_event_group = xEventGroupCreate();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}
/**
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO status:
 * GPIO18: output (ESP32C2/ESP32H2 uses GPIO8 as the second output pin)
 * GPIO19: output (ESP32C2/ESP32H2 uses GPIO9 as the second output pin)
 * GPIO4:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO5:  input, pulled up, interrupt from rising edge.
 *
 * Note. These are the default GPIO pins to be used in the example. You can
 * change IO pins in menuconfig.
 *
 * Test:
 * Connect GPIO18(8) with GPIO4
 * Connect GPIO19(9) with GPIO5
 * Generate pulses on GPIO18(8)/19(9), that triggers interrupt on GPIO4/5
 *
 */

#define GPIO_OUTPUT_IO_0    CONFIG_GPIO_OUTPUT_0
#define GPIO_OUTPUT_IO_1    CONFIG_GPIO_OUTPUT_1
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))
/*
 * Let's say, GPIO_OUTPUT_IO_0=18, GPIO_OUTPUT_IO_1=19
 * In binary representation,
 * 1ULL<<GPIO_OUTPUT_IO_0 is equal to 0000000000000000000001000000000000000000 and
 * 1ULL<<GPIO_OUTPUT_IO_1 is equal to 0000000000000000000010000000000000000000
 * GPIO_OUTPUT_PIN_SEL                0000000000000000000011000000000000000000
 * */
#define GPIO_INPUT_IO_0     CONFIG_GPIO_INPUT_0
#define GPIO_INPUT_IO_1     CONFIG_GPIO_INPUT_1
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
/*
 * Let's say, GPIO_INPUT_IO_0=4, GPIO_INPUT_IO_1=5
 * In binary representation,
 * 1ULL<<GPIO_INPUT_IO_0 is equal to 0000000000000000000000000000000000010000 and
 * 1ULL<<GPIO_INPUT_IO_1 is equal to 0000000000000000000000000000000000100000
 * GPIO_INPUT_PIN_SEL                0000000000000000000000000000000000110000
 * */
#define ESP_INTR_FLAG_DEFAULT 0

#define DEBOUNCE_DELAY_MS 50

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
SemaphoreHandle_t xSemaphore = NULL;

gpio_config_t io_conf = {};

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg)
{
    const int TOPIC_LEN = strlen(TOPIC);
    uint32_t io_num;
    TickType_t last_isr_time = 0;
    uint32_t no_of_cars=0;

    char payload[100];
    IoT_Publish_Message_Params param_QOS1;
    

    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {

            TickType_t current_time = xTaskGetTickCount();

            if ((current_time - last_isr_time) >= pdMS_TO_TICKS(DEBOUNCE_DELAY_MS))
            {


                // Your debounced ISR code
                //printf("GPIO[%" PRIu32 "] intr, val: %d\n", io_num, gpio_get_level(io_num));
                if(gpio_get_level(io_num) == 1){
                    printf("no_of_cars = %" PRIu32 " \n", ++no_of_cars);
                    sprintf(payload, "%s %" PRIu32 " ", "Number Of Cars = ",no_of_cars);
                    param_QOS1.qos = QOS1;
                    param_QOS1.payload = (void *) payload;
                    param_QOS1.isRetained = 0;
                    param_QOS1.payloadLen = strlen(payload);

                    rc = aws_iot_mqtt_publish(&client, TOPIC, TOPIC_LEN, &param_QOS1);
                    if (rc == MQTT_REQUEST_TIMEOUT_ERROR) {
                        ESP_LOGW(TAG, "QOS1 publish ack not received.");
                        rc = SUCCESS;
                    }

                }
                    

                last_isr_time = current_time;
            }
        }
    }
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    initialise_wifi();
    xTaskCreatePinnedToCore(&aws_iot_task, "aws_iot_task", 9216, NULL, 5, NULL, 1);
    //zero-initialize the config structure.
    //gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    // //interrupt of rising edge
    // io_conf.intr_type = GPIO_INTR_NEGEDGE;
    // //bit mask of the pins, use GPIO4/5 here
    // //io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // io_conf.pin_bit_mask = GPIO_INPUT_IO_0;
    // //set as input mode
    // io_conf.mode = GPIO_MODE_INPUT;
    // //enable pull-up mode
    // io_conf.pull_up_en = 1;
    // gpio_config(&io_conf);

    //change gpio interrupt type for one pin
    gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_NEGEDGE);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);

    //remove isr handler for gpio number.
    gpio_isr_handler_remove(GPIO_INPUT_IO_0);
    //hook isr handler for specific gpio pin again
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);

    printf("Minimum free heap size: %"PRIu32" bytes\n", esp_get_minimum_free_heap_size());

    int cnt = 0;
    while(1) {
        //printf("cnt: %d\n", cnt++);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_OUTPUT_IO_0, cnt % 2);
        gpio_set_level(GPIO_OUTPUT_IO_1, cnt % 2);
    }
}
