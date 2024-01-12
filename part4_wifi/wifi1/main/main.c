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


/**
 * @brief Starting point function
 *
 */


void MyApplication()
{
  printf("lab7 wifi_connected \n\r");
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