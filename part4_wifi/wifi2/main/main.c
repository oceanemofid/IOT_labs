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

#include "wifi_connect.h"

//static const uint32_t STACK_SIZE = 4000;
//static const uint32_t vTaskConnected = 5;

static const char *TAG = "WIFI_LAB";
int cpt=0;
int nbEvent=0;

/* To be added */
 #include "esp_http_client.h"
 

/**
 * @brief The HTTP event handler
 * 
 * @param evt HTTP Client events data
 * @return esp_err_t ESP_OK if no error else ESP_FAIL
 */
esp_err_t wifi_http_event_handler_cb(esp_http_client_event_t *evt) {
  

  switch (evt->event_id) {

    case HTTP_EVENT_ON_CONNECTED:
      ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
      break;

    case HTTP_EVENT_HEADER_SENT:
      ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
      cpt =0;
      nbEvent = 0;
      break;    

    case HTTP_EVENT_ON_HEADER:
      ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
      printf("%.*s", evt->data_len, (char*)evt->data);
      break;  

    case HTTP_EVENT_ON_DATA:
      ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA Len=%d", evt->data_len);
      printf("%.*s\n", evt->data_len, (char *)evt->data);
      cpt += evt->data_len;
      nbEvent++;
      break;

    case HTTP_EVENT_ON_FINISH:
      ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
      printf("nb event : %d total content lenght : %d\n",nbEvent, cpt);
      break;

    case HTTP_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
      break;

    case HTTP_EVENT_ERROR:
      ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
      return ESP_FAIL;
      break;

    default:
      ESP_LOGW(TAG, "HTTP_EVENT UNDEFINED EVENT");
      break;
  }
  return ESP_OK;
}

/**
 * @brief Fetch the HTTP data from an URL
 * 
 * @param url the text url
 */
void fetchHttpData(const char *url) {

  /* HTTP session init */
  esp_http_client_config_t clientConfig = {
      .url = url,
      .event_handler = wifi_http_event_handler_cb};

  esp_http_client_handle_t client = esp_http_client_init(&clientConfig);

  /* Perform HTTP session in blocking mode. Wait until http session is finished */
  esp_err_t err = esp_http_client_perform(client);

  /* Print information as soon as the session is finished */
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "HTTP GET status = %d, content_length = %d",
              esp_http_client_get_status_code(client),
              esp_http_client_get_content_length(client));
  }
  else {
      ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
  }

  /* Clean the client session */
  esp_http_client_cleanup(client);  
}


/**
 * @brief Starting point function
 *
 */


void MyApplication()
{
  printf("lab7 wifi_connected \n\r");
  fetchHttpData("http://www.google.com");
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