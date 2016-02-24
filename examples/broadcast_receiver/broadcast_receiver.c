/*
 * Simple cunter that is displayed on 16lf01 VFD display.
 * Author: Grzegorz Hetman - ghetman@gmail.com
 */

#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "lwip/api.h"
#include <string.h>
#include "ssid_config.h"

void broadcast_receiver(void *pvParameters) {
   
    err_t err;

    static char buffer[4096];

    vTaskDelay(15000 / portTICK_RATE_MS);
    
    while(1) {

        struct netconn* conn;

        // Create UDP connection
        conn = netconn_new(NETCONN_UDP);

        // Connect to local port
        err = netconn_bind(conn, IP_ADDR_ANY, 8005);

        if (err != ERR_OK) {
            netconn_delete(conn);
            printf("%s : Could not bind! (%s)\n", __FUNCTION__, lwip_strerr(err));
            continue;
        }

        for(;;) {
            static struct netbuf *buf;
            err = netconn_recv(conn, &buf);
            netbuf_copy(buf, buffer, buf->p->tot_len);
            buffer[buf->p->tot_len] = '\0';
            
            if (err != ERR_OK) {
                printf("%s : Could not receive data!!! (%s)\n", __FUNCTION__, lwip_strerr(err));
                continue;
            }
            printf("GET DATA: %s\n", buffer);
            // De-allocate packet buffer
            netbuf_delete(buf); 
        }

        err = netconn_disconnect(conn);
        printf("%s : Disconnected from IP_ADDR_BROADCAST (%s)\n", __FUNCTION__, lwip_strerr(err));

        err = netconn_delete(conn);
        printf("%s : Deleted connection (%s)\n", __FUNCTION__, lwip_strerr(err));
        vTaskDelay(1000/portTICK_RATE_MS);
    }
}

void user_init(void) {
    uart_set_baud(0, 115200);
    
    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };

    // Required to call wifi_set_opmode before station_set_config.
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    xTaskCreate(&broadcast_receiver, (signed char *)"broadcast_receiver", 256, NULL, 2, NULL);
}