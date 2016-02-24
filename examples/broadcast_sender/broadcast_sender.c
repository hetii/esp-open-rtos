/*
 * This is example of udp broadcaster.
 * It grab data from ds18b20 sensor and emit result over wifi.
 *
 * Author Grzegorz Hetman : ghetman@gmail.com
 * License public domain/CC0
 */

#include <string.h>

#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "lwip/api.h"
#include "ssid_config.h"

void broadcast_sender(void *pvParameters)
{
    char msg[100];

    // Broadcaster part
    err_t err;
    // Initialize one wire bus.
    onewire_init(GPIO_FOR_ONE_WIRE);

    while(1) {

        // Send out some UDP data
        struct netconn* conn;

        // Create UDP connection
        conn = netconn_new(NETCONN_UDP);

        // Connect to local port
        err = netconn_bind(conn, IP_ADDR_ANY, 8004);

        if (err != ERR_OK) {
            netconn_delete(conn);
            printf("%s : Could not bind! (%s)\n", __FUNCTION__, lwip_strerr(err));
            continue;
        }

        err = netconn_connect(conn, IP_ADDR_BROADCAST, 8005);

        if (err != ERR_OK) {
            netconn_delete(conn);
            printf("%s : Could not connect! (%s)\n", __FUNCTION__, lwip_strerr(err));
            continue;
        }

        for(;;) {
            
            for (int i = 0; i < amount; ++i)
            {
            
                sprintf(msg, "Sensor %d report: %d.%02d ""\xC2""\xB0""C\n",t[i].id, intpart, fraction);
                printf("%s", msg);

                struct netbuf* buf = netbuf_new();
                void* data = netbuf_alloc(buf, strlen(msg));

                memcpy (data, msg, strlen(msg));
                err = netconn_send(conn, buf);

                if (err != ERR_OK) {
                    printf("%s : Could not send data!!! (%s)\n", __FUNCTION__, lwip_strerr(err));
                    continue;
                }
                // De-allocate packet buffer
                netbuf_delete(buf); 
            }
            vTaskDelay(1000/portTICK_RATE_MS);
        }

        err = netconn_disconnect(conn);
        printf("%s : Disconnected from IP_ADDR_BROADCAST (%s)\n", __FUNCTION__, lwip_strerr(err));

        err = netconn_delete(conn);
        printf("%s : Deleted connection (%s)\n", __FUNCTION__, lwip_strerr(err));

        vTaskDelay(1000/portTICK_RATE_MS);
    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);

    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    // Set led to indicate wifi status.
    sdk_wifi_status_led_install(2, PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);

    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };

    // Required to call wifi_set_opmode before station_set_config.
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    xTaskCreate(&broadcast_sender, (signed char *)"broadcast_sender", 256, NULL, 2, NULL);
}

