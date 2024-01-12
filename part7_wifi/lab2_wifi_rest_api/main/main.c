/****************************************************************************
 * Copyright (C) 2021 by Fabrice Muller                                     *
 *                                                                          *
 * This file is useful for ESP32 Design course.                             *
 *                                                                          *
 ****************************************************************************/

/**
 * @file main.c
 * @author Fabrice Muller
 * @date 12 Nov. 2021
 * @brief File containing the lab1-1 of Part 7.
 *
 * @see https://github.com/fmuller-pns/esp32-vscode-project-template
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "http_data.h"

#include "wifi_connect.h"
#include "cJSON.h"

//static const uint32_t STACK_SIZE = 4000;
//static const uint32_t vTaskConnected = 5;

static const char *TAG = "WIFI_LAB";

/* To be added */
#include "esp_http_client.h"

/* openweathermap API URL for Cannes city, Unit = degree */
const char *CITY = "Cannes";
const char *OPEN_WEATHER_MAP_URL = "http://api.openweathermap.org/data/2.5/weather?q=Cannes&appid=bfaf90865d45e39c390da17ffa61e195&units=metric";

/* Example of response for testing the extractJSONWeatherMapInformation() function */
const char *RESP_EXAMPLE = "{\"coord\":{\"lon\":7.0167,\"lat\":43.55},\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01d\"}],\"base\":\"stations\",\"main\":{\"temp\":24.72,\"feels_like\":24.84,\"temp_min\":23.12,\"temp_max\":25.74,\"pressure\":1019,\"humidity\":61},\"visibility\":10000,\"wind\":{\"speed\":3.6,\"deg\":170},\"clouds\":{\"all\":0},\"dt\":1633099464,\"sys\":{\"type\":1,\"id\":6507,\"country\":\"FR\",\"sunrise\":1633066158,\"sunset\":1633108421},\"timezone\":7200,\"id\":6446684,\"name\":\"Cannes\",\"cod\":200}";

/* Sensor information */
#define WEATHERMAPINFO_DESCRIPTION_LENGTH 100

typedef struct {
  double latitude;
  double longitude;
  double temperature;
  double temp_min;
  double temp_max;
  int pressure;
  int humidity;
  char description[WEATHERMAPINFO_DESCRIPTION_LENGTH];
} weathermapinfo_t;

extern xSemaphoreHandle getConnectionWifiSemaphore();

//Function prototypes
void connectedWifiTask(void *para);
void extractJSONWeatherMapInformation(char *resp, weathermapinfo_t *weathermapinfo);

/**
 * @brief Starting point function
 *
 */

//---------------------------------2.1.2------------------------------------
/*
void MyApplication()
{
  printf("lab7 wifi_connected \n\r");vConnectedWifi
    param.buffer = NULL; // Set the buffer pointer to NULL after freeing the memory
  }
}


void vConnectedWifi(){
  while(pdTRUE){
    if(xSemaphoreTake(getConnectionWifiSemaphore(),pdMS_TO_TICKS(10000))==pdTRUE)
    {
      printf("Connected on %s\n\r",WIFI_SSID);
      printf("Run application\n\r");

      MyApplication();

      xSemaphoreTake(getConnectionWifiSemaphore(),portMAX_DELAY);
      printf("Retried connection on %s\n\r",WIFI_SSID);
    }
    else
    {
      printf("Failed to connect. Retry in \n\r");
      for(int i =5;i>0;i--)
      {
        printf("... %d \n\r",i);
        vTaskDelay(pdMS_TO_TICKS(1000));
      }
      esp_restart();
    }
  }
}

void app_main() {
  //ERROR, WARNING, INFO level log 
  esp_log_level_set(TAG, ESP_LOG_INFO);
 
  //Init WiFi 
  wifiInit();

  //Create connected WiFi Task, STACK=3*1024, Priority=5
  xTaskCreate(vConnectedWifi, "vConnectedWifi", 3*1024, (void*)"vConnectedWifi", 5, NULL);
 

  //Delete task 
  vTaskDelete(NULL);
}*/

//---------------------------------------2.1.3---------------------------------------
void connectedWifiTask(void *para) {
    ESP_LOGI(TAG, "Waiting for Wi-Fi connection...");

    // Wait for the Wi-Fi to be connected
    if (xSemaphoreTake(getConnectionWifiSemaphore(), pdMS_TO_TICKS(10000))) {
        ESP_LOGI(TAG, "Connected to Wi-Fi. Preparing to fetch weather data...");

        // Construct the full URL for the OpenWeatherMap API
        char *url = malloc(strlen(OPEN_WEATHER_MAP_URL) + strlen(CITY) + 1);
        if (url == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for URL");
            vTaskDelete(NULL); // Stop the task if we cannot allocate memory
        }
        sprintf(url, OPEN_WEATHER_MAP_URL, CITY);

        // Assume that http_param_t and fetchHttpData are defined and work as intended
        http_param_t http_param;
        http_param.buffer = NULL;
        http_param.index = 0;

        // Fetch data from OpenWeatherMap API
        if (fetchHttpData(&http_param, url) == ESP_OK) {
            ESP_LOGI(TAG, "HTTP Data fetched successfully");

            // Extract weather information
            weathermapinfo_t weathermapinfo;
            extractJSONWeatherMapInformation(http_param.buffer, &weathermapinfo);

            // Print the weather information
            ESP_LOGI(TAG, "Weather at %s:", CITY);
            ESP_LOGI(TAG, "(Latitude,Longitude) = (%f,%f)", weathermapinfo.latitude, weathermapinfo.longitude);
            ESP_LOGI(TAG, "Description: %s", weathermapinfo.description);
            ESP_LOGI(TAG, "Temperature: %.2f°C", weathermapinfo.temperature);
            ESP_LOGI(TAG, "Temperature (Min): %.2f°C", weathermapinfo.temp_min);
            ESP_LOGI(TAG, "Temperature (Max): %.2f°C", weathermapinfo.temp_max);
            ESP_LOGI(TAG, "Pressure: %d hPa", weathermapinfo.pressure);
            ESP_LOGI(TAG, "Humidity: %d%%", weathermapinfo.humidity);
        } else {
            ESP_LOGE(TAG, "Failed to fetch HTTP Data");
        }

        // Free the allocated buffer after use
        if (http_param.buffer != NULL) {
            free(http_param.buffer);
        }

        // Free the allocated URL
        free(url);

        xSemaphoreTake(getConnectionWifiSemaphore(),portMAX_DELAY);
        printf("Retried connection on %s\n\r",WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "Failed to connect to Wi-Fi");
        printf("Failed to connect. Retry in \n\r");
        for(int i =5;i>0;i--)
        {
            printf("... %d \n\r",i);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
         esp_restart();
    }

    // Delete the task if it's no longer needed
    vTaskDelete(NULL);
}


void extractJSONWeatherMapInformation(char *resp, weathermapinfo_t *weathermapinfo) {

    /* Convert textual response to JSON object */
    cJSON *payload = cJSON_Parse(resp);
    if (!payload) {
        ESP_LOGE(TAG, "Error before: %s", cJSON_GetErrorPtr());
        return;
    }

    /* Extract coordinates */
    cJSON *coord = cJSON_GetObjectItemCaseSensitive(payload, "coord");
    if (coord) {
        cJSON *longitude = cJSON_GetObjectItemCaseSensitive(coord, "lon");
        cJSON *latitude = cJSON_GetObjectItemCaseSensitive(coord, "lat");
        if (longitude && cJSON_IsNumber(longitude)) {
            weathermapinfo->longitude = longitude->valuedouble;
        }
        if (latitude && cJSON_IsNumber(latitude)) {
            weathermapinfo->latitude = latitude->valuedouble;
        }
    }

    /* Extract temperature, pressure, and humidity */
    cJSON *app_main = cJSON_GetObjectItemCaseSensitive(payload, "main");
    if (app_main) {
        cJSON *temp = cJSON_GetObjectItemCaseSensitive(app_main, "temp");
        cJSON *temp_min = cJSON_GetObjectItemCaseSensitive(app_main, "temp_min");
        cJSON *temp_max = cJSON_GetObjectItemCaseSensitive(app_main, "temp_max");
        cJSON *pressure = cJSON_GetObjectItemCaseSensitive(app_main, "pressure");
        cJSON *humidity = cJSON_GetObjectItemCaseSensitive(app_main, "humidity");
       
        if (temp && cJSON_IsNumber(temp)) {
            weathermapinfo->temperature = temp->valuedouble;
        }
        if (temp_min && cJSON_IsNumber(temp_min)) {
            weathermapinfo->temp_min = temp_min->valuedouble;
        }
        if (temp_max && cJSON_IsNumber(temp_max)) {
            weathermapinfo->temp_max = temp_max->valuedouble;
        }
        if (pressure && cJSON_IsNumber(pressure)) {
            weathermapinfo->pressure = pressure->valueint;
        }
        if (humidity && cJSON_IsNumber(humidity)) {
            weathermapinfo->humidity = humidity->valueint;
        }
    }

    /* Extract weather description */
    cJSON *weather = cJSON_GetObjectItemCaseSensitive(payload, "weather");
    if (weather && cJSON_IsArray(weather)) {
        cJSON *weather_item = cJSON_GetArrayItem(weather, 0);
        if (weather_item) {
            cJSON *description = cJSON_GetObjectItemCaseSensitive(weather_item, "description");
            if (description && cJSON_IsString(description)) {
                strncpy(weathermapinfo->description, description->valuestring, WEATHERMAPINFO_DESCRIPTION_LENGTH - 1);
                weathermapinfo->description[WEATHERMAPINFO_DESCRIPTION_LENGTH - 1] = '\0'; // Ensure null termination
            }
        }
    }

    /* Free memory */
    cJSON_Delete(payload);
}

void app_main() {
  //ERROR, WARNING, INFO level log 
  esp_log_level_set(TAG, ESP_LOG_INFO);
 
  //Init WiFi 
  wifiInit();

  //Create connected WiFi Task, STACK=3*1024, Priority=5
  xTaskCreate(connectedWifiTask, "connectedWifiTask", 3*1024, (void*)"connectedWifiTask", 5, NULL);
 

  //Delete task 
  vTaskDelete(NULL);
}