/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"
#include "driver/i2c_master.h"

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "10.0.0.195"
#define WEB_PORT "8000"
#define WEB_PATH "/"

#define I2C_MASTER_SCL_IO           8       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           10      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0                   /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ          400000 /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define SHTC3_ADDR         0x70        /*!< Address of the SHTC3 */
#define MPU9250_WHO_AM_I_REG_ADDR   0x75        /*!< Register addresses of the "who am I" register */
#define MPU9250_PWR_MGMT_1_REG_ADDR 0x6B        /*!< Register addresses of the power management register */
#define MPU9250_RESET_BIT           7

#define TRIG_GPIO  GPIO_NUM_2
#define ECHO_GPIO  GPIO_NUM_3

#define MAX_TIMEOUT_US 100000
#define PAYLOAD_MAX_LEN 128

static const char *TAG = "http_post_task";

// static const char *PAYLOAD = "{\"humidity\":,\"temperature\":}";
char payload[PAYLOAD_MAX_LEN];

void shtc3_read_data(i2c_master_dev_handle_t dev_handle, uint16_t * temperature, uint16_t * humidity) {
    uint8_t data[6] = { 0 };

    esp_err_t err = i2c_master_receive(
        dev_handle,
        data,
        sizeof(data),
        I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS
    );

     // Combine bytes to form the raw values (ignore the CRC bytes)
     uint16_t raw_temperature  = ((uint16_t)data[0] << 8) | data[1];
     uint16_t raw_humidity = ((uint16_t)data[3] << 8) | data[4];
 
     *humidity = raw_humidity;
     *temperature = raw_temperature;
}

esp_err_t shtc3_send_command(uint16_t cmd, i2c_master_dev_handle_t dev_handle) {
    uint8_t data[2] = { (uint8_t)((cmd&0xff00)>>8), (uint8_t)(cmd&0xff) };
    return i2c_master_transmit(
        dev_handle,
        data,
        sizeof(data),
        I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS
    );

}



/**
 * @brief i2c master initialization
 */
static void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SHTC3_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_config, dev_handle));

    
}


float read_temp(i2c_master_dev_handle_t dev_handle) {
    ESP_ERROR_CHECK(shtc3_send_command(0x3517, dev_handle));
        vTaskDelay(pdMS_TO_TICKS(12)); // Give a small delay after wakeup

        
        ESP_ERROR_CHECK(shtc3_send_command(0x7CA2, dev_handle));
        
        vTaskDelay(pdMS_TO_TICKS(12));

        
        uint16_t raw_temperature = 0, raw_humidity = 0;
        shtc3_read_data(dev_handle, &raw_temperature, &raw_humidity);

        
        float temperature = -45.0f + 175.0f * (raw_temperature / 65536.0f);
        // float humidity    = 100.0f * (raw_humidity / 65536.0f);
        // float temperatureF = (temperature * 1.8) + 32;

        
        // 5. Put the sensor to sleep to conserve power
        ESP_ERROR_CHECK(shtc3_send_command(0xB098, dev_handle));
        vTaskDelay(pdMS_TO_TICKS(15));
        return temperature;
}

float read_hum(i2c_master_dev_handle_t dev_handle) {
    ESP_ERROR_CHECK(shtc3_send_command(0x3517, dev_handle));
        vTaskDelay(pdMS_TO_TICKS(12)); // Give a small delay after wakeup

        
        ESP_ERROR_CHECK(shtc3_send_command(0x7CA2, dev_handle));
        
        vTaskDelay(pdMS_TO_TICKS(12));

        
        uint16_t raw_temperature = 0, raw_humidity = 0;
        shtc3_read_data(dev_handle, &raw_temperature, &raw_humidity);

        
        // float temperature = -45.0f + 175.0f * (raw_temperature / 65536.0f);
        float humidity    = 100.0f * (raw_humidity / 65536.0f);
        // float temperatureF = (temperature * 1.8) + 32;

        
        // 5. Put the sensor to sleep to conserve power
        ESP_ERROR_CHECK(shtc3_send_command(0xB098, dev_handle));
        vTaskDelay(pdMS_TO_TICKS(15));
        return humidity;
}

void make_payload(i2c_master_dev_handle_t dev) {
    
    float h = read_hum(dev);
    float t = read_temp(dev);

    // Format as JSON: {"humidity":42.50,"temperature":23.10}
    int len = snprintf(payload, PAYLOAD_MAX_LEN,
                       "{\"humidity\":%.2f,\"temperature\":%.2f}",
                       h, t);

    if (len < 0 || len >= PAYLOAD_MAX_LEN) {
        // handle error: truncated or encoding failure
        return;
    }

    
}


static void http_post_task(i2c_master_dev_handle_t dev_handle)
{
    
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int sock = -1;
    char recv_buf[64];

    while (1) {
        make_payload(dev_handle);
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);
        if (err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        sock = socket(res->ai_family, res->ai_socktype, 0);
        if (sock < 0) {
            ESP_LOGE(TAG, "Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "Socket connect failed errno=%d", errno);
            close(sock);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "Connected to server");
        freeaddrinfo(res);

        int payload_len = strlen(payload);
        /* Build the POST request */
        char request[256];
        int req_len = snprintf(request, sizeof(request),
            "POST %s HTTP/1.0\r\n"
            "Host: %s:%s\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s",
            WEB_PATH, WEB_SERVER, WEB_PORT,
            payload_len,
            payload);

        if (req_len < 0 || req_len >= sizeof(request)) {
            ESP_LOGE(TAG, "Request buffer too small (needed %d bytes)", req_len);
            close(sock);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Send POST request */
        if (write(sock, request, req_len) < 0) {
            ESP_LOGE(TAG, "Socket send failed");
            close(sock);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "POST request sent successfully");

        /* Set receive timeout */
        struct timeval timeout = { .tv_sec = 5, .tv_usec = 0 };
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        /* Read HTTP response */
        int r;
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(sock, recv_buf, sizeof(recv_buf) - 1);
            if (r > 0) {
                printf("%.*s", r, recv_buf);
            }
        } while (r > 0);
        printf("\n");

        ESP_LOGI(TAG, "Finished reading response. last read=%d errno=%d", r, errno);
        close(sock);

        /* Delay before next POST */
        for (int countdown = 1; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d...", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting POST again!");
    }
}



void app_main(void)
{
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    
    i2c_master_init(&bus_handle, &dev_handle);
    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    
    
    http_post_task(dev_handle);

    
    
}
