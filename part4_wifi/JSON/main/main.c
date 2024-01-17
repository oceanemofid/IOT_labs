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

//static const uint32_t STACK_SIZE = 4000;
//static const uint32_t vTaskConnected = 5;

static const char *TAG = "WIFI_LAB";

/* To be added */
 #include "esp_http_client.h"
 #include "cJSON.h"
 
/**
 * @brief Starting point function
 *
 */

/* openweathermap API URL for Cannes city, Unit = degree */
const char *CITY = "Cannes";
const char *OPEN_WEATHER_MAP_URL = "http://api.openweathermap.org/data/2.5/weather?q=Cannes&appid=bfaf90865d45e39c390da17ffa61e195&units=metric";


/* Example of response for testing the extractJSONWeatherMapInformation() function */
const char *RESP_EXAMPLE = "{\"coord\":{\"lon\":7.0167,\"lat\":43.55},\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01d\"}],\"base\":\"stations\",\"main\":{\"temp\":24.72,\"feels_like\":24.84,\"temp_min\":23.12,\"temp_max\":25.74,\"pressure\":1019,\"humidity\":61},\"visibility\":10000,\"wind\":{\"speed\":3.6,\"deg\":170},\"clouds\":{\"all\":0},\"dt\":1633099464,\"sys\":{\"type\":1,\"id\":6507,\"country\":\"FR\",\"sunrise\":1633066158,\"sunset\":1633108421},\"timezone\":7200,\"id\":6446684,\"name\":\"Cannes\",\"cod\":200}";

/* Sensor information */
# define WEATHERMAPINFO_DESCRIPTION_LENGTH 100

typedef struct {
  float latitude;
  float longitude;
  float temp;
  float feels_like;
  float temp_min;
  float temp_max;
  float pressure;
  float humidity;
  float visibility;
  char description[WEATHERMAPINFO_DESCRIPTION_LENGTH];

} weathermapinfo_t;


/**
 * @brief Parse the JSON Open Weather Map Information and extract useful information to print it.
 * 
 * @param resp textual response of the Open Weather Map server 
 */
void extractJSONWeatherMapInformation(char *resp, weathermapinfo_t *weathermapinfo) {

    /* Convert textual resp to JSON object */
    cJSON *payload = cJSON_Parse(resp);

    /* Coordonate (JSon Items)43.550000,7.016700 */
    cJSON *coord = cJSON_GetObjectItem(payload, "coord");   
    cJSON *longitude = cJSON_GetObjectItem(coord, "lon");
    cJSON *latitude = cJSON_GetObjectItem(coord, "lat");

    /* Set information in the structure */
    weathermapinfo->latitude = latitude->valuedouble;
    weathermapinfo->longitude = longitude->valuedouble;   

    cJSON *main1 = cJSON_GetObjectItem(payload, "main");   
    cJSON *temp = cJSON_GetObjectItem(main1, "temp"); 
    cJSON *feels_like = cJSON_GetObjectItem(main1, "feels_like");
    cJSON *temp_min = cJSON_GetObjectItem(main1, "temp_min");
    cJSON *temp_max = cJSON_GetObjectItem(main1, "temp_max");
    cJSON *pressure = cJSON_GetObjectItem(main1, "pressure");
    cJSON *humidity = cJSON_GetObjectItem(main1, "humidity");
    cJSON *visibility = cJSON_GetObjectItem(payload, "visibility");

    weathermapinfo->temp = temp->valuedouble;
    weathermapinfo->feels_like = feels_like->valuedouble;
    weathermapinfo->temp_min = temp_min->valuedouble;
    weathermapinfo->temp_max = temp_max->valuedouble;
    weathermapinfo->pressure = pressure->valuedouble;
    weathermapinfo->humidity = humidity->valuedouble;
    weathermapinfo->visibility = visibility->valuedouble;

    /* Free memory */
    cJSON_Delete(payload);
}

void MyApplication()
{
  
  http_param_t http_param;
  fetchHttpData(&http_param, OPEN_WEATHER_MAP_URL);
  printf("%s\n", (char *)http_param.buffer);

  char *url = malloc(strlen(OPEN_WEATHER_MAP_URL) + strlen(CITY) + 1);
  sprintf(url, OPEN_WEATHER_MAP_URL);
  
  /* Extract openweathermap information from response */
  weathermapinfo_t weathermapinfo;
  extractJSONWeatherMapInformation(http_param.buffer, &weathermapinfo);

  printf("METEO at %s\n", CITY);
  printf("(latitude,longitude) = (%f,%f)\n", weathermapinfo.latitude, weathermapinfo.longitude);
  printf("(temperature,feels_like) = (%f,%f)\n", weathermapinfo.feels_like, weathermapinfo.feels_like);
  printf("(temp_min,temp_max) = (%f,%f)\n", weathermapinfo.temp_min, weathermapinfo.temp_max);
  printf("(pressure) = (%f)\n", weathermapinfo.pressure);
  printf("(humidity) = (%f)\n", weathermapinfo.humidity);
  printf("(visibility) = (%f)\n", weathermapinfo.visibility);
  free(url);

  if(http_param.buffer != NULL){
    free(http_param.buffer);
    
  }
}

void vConnectedWifi(){
  

  while(1){
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
  /* ERROR, WARNING, INFO level log */
  esp_log_level_set(TAG, ESP_LOG_INFO);
 
  /* Init WiFi */
  wifiInit();

  /* Create connected WiFi Task, STACK=3*1024, Priority=5 */
  xTaskCreate(vConnectedWifi, "vConnectedWifi", 3*1024, (void*)"vConnectedWifi", 5, NULL);
 

  /* Delete task */
  vTaskDelete(NULL);
}