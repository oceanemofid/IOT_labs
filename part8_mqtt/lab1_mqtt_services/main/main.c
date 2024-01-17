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
#include "esp_http_client.h"
#include "esp_sntp.h"
#include "cJSON.h"

#include "wifi_connect.h"
#include "http_data.h"
//#include "ntp_time.h"
#include "mqtt_tcp.h"


static const char *TAG = "MQTT_MAIN";
extern xSemaphoreHandle getConnectionWifiSemaphore();


//Function prototypes
void connectedWifiTask(void *para);
void testMqttTask(void *para);

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

void connectedWifiTask(void *para) {
    /* Run application */
    printf("Run Application\n");

    ESP_LOGI(TAG, "Waiting for Wi-Fi connection...");

    // Wait for the Wi-Fi to be connected
    if (xSemaphoreTake(getConnectionWifiSemaphore(), pdMS_TO_TICKS(10000))) {
        ESP_LOGI(TAG, "Connected to Wi-Fi.");
    
    xSemaphoreTake(getConnectionWifiSemaphore(),pdMS_TO_TICKS(10000));
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

    // Start MQTT client (details provided in the previous snippets)
    esp_mqtt_client_handle_t mqtt_client = mqtt_start("mqtt://134.59.213.24:1883"); // Example broker URL

    // Get MQTT connection event (semaphore)
    xSemaphoreHandle connectionMqttSemaphore = getConnectionMqttSemaphore();

    // Wait on MQTT Connected event
    if(xSemaphoreTake(connectionMqttSemaphore, portMAX_DELAY)) {
        ESP_LOGI(TAG, "MQTT Connected");

        // Create Test Task (testMqttTask), priority = 6, parameter = MQTT Client, stack size = 3*1024
        xTaskCreate(&testMqttTask, "testMqttTask", 3*1024, (void*)mqtt_client, 6, NULL);
    } else {
        ESP_LOGE(TAG, "MQTT Connection Failed");
    }

    // Blocked until loss connection and retry connection
    xSemaphoreTake(connectionMqttSemaphore, portMAX_DELAY);

    // Delete the task if it's no longer needed
    vTaskDelete(NULL);
}

void testMqttTask(void *para) {

  ESP_LOGI(TAG, "Starting testMqttTask");

  /* Get MQTT Client passed by task parameter */
  esp_mqtt_client_handle_t mqtt_client = (esp_mqtt_client_handle_t)para;

  /* Subscription of the End-Node : Sub_AREA1 */
  // esp_mqtt_client_subscribe()
  int msg_id_area1 = esp_mqtt_client_subscribe(mqtt_client, "Polytech/AREA1/#", 0);
  if (msg_id_area1 != -1)
    {
      ESP_LOGI(TAG, "Sent subscribe successful, msg_id=%d", msg_id_area1);
    }

    else 
    {
      ESP_LOGE(TAG, "Failed to send subscribe");
    }


  /* Publishing temperature with the End-Node : Pub_E110_Room */
  // esp_mqtt_client_publish()
  const char* temp_data = "23";
  int msg_id_e110 = esp_mqtt_client_publish(mqtt_client,"Polytech/AREA1/E110/temperature", temp_data, 0, 1, 0);
  if (msg_id_e110 != -1)
  {
    ESP_LOGI(TAG, "Published temperature to Pub_E110_Room, msg_id=%d", msg_id_e110);
  }

  else 
  {
    ESP_LOGE(TAG, "Failed to publish temperature to Pub_E110_Room");
  }

  /* Wait 3 sec. */
  vTaskDelay(pdMS_TO_TICKS(3000));

  /* Publishing humidity AND temperature with the End-Node : Pub_Sxxx_Room */
  const char* temp_1_data = "24.0";
  const char* hum_1_data = "40.0";
  const char* temp_2_data = "35.0";
  const char* hum_2_data = "50.0";

  int msg_id1 = esp_mqtt_client_publish(mqtt_client, "Polytech/AREA1/S113/temerature", temp_1_data, 0, 1, 0);
  int msg_id2 = esp_mqtt_client_publish(mqtt_client, "Polytech/AREA1/S113/humidity", hum_1_data, 0, 1, 0);
  int msg_id3 = esp_mqtt_client_publish(mqtt_client, "Polytech/AREA1/S218/temerature", temp_2_data, 0, 1, 0);
  int msg_id4 = esp_mqtt_client_publish(mqtt_client, "Polytech/AREA1/S218/humidity", hum_2_data, 0, 1, 0);

  if (msg_id1 != -1 || msg_id2 != -1 || msg_id3 != -1 ||msg_id4 != -1)
  {
    ESP_LOGI(TAG, "Published temp and hum to Sxxx_Room, msg_id1=%d, msg_id2=%d, msg_id3=%d, msg_id4=%d", msg_id1, msg_id2, msg_id3, msg_id4);
  }    

  else 
  {
    ESP_LOGE(TAG, "Failed to publish temp and humidity to Pub_Sxxx_Room");
  }


  /* Subscription of the End-Node : Sub_E110 */
  int msg_id_e110_2 = esp_mqtt_client_subscribe(mqtt_client, "Polytech/AREA1/E110/+", 0);
  if (msg_id_e110_2 != -1)
  {
    ESP_LOGI(TAG, "Sent subscribe successful, msg_id=%d", msg_id_e110_2);
  }

  else 
  {
    ESP_LOGE(TAG, "Failed to send subscribe to Sub_E110");
  }

  /* Publishing temperature with the End-Node : Pub_E110_Room */
  int msg_id_e110_3 = esp_mqtt_client_publish(mqtt_client, "Polytech/AREA1/E110/temperature", temp_data, 0, 1, 0);
  if (msg_id_e110_3 != -1)
  {
    ESP_LOGI(TAG, "Sent publish temp E110 successful, msg_id=%d", msg_id_e110_3);
  }

  else 
  {
  ESP_LOGE(TAG, "Failed to send publish to Pub_E110");
  }

  /* Wait 3 sec. */
  vTaskDelay(pdMS_TO_TICKS(3000));

  /* Publishing humidity AND temperature with the End-Node : Pub_Sxxx_Room */
  int msg_id1_2 = esp_mqtt_client_publish(mqtt_client, "Polytech/AREA1/S113/temerature", temp_1_data, 0, 1, 0);
  int msg_id2_2 = esp_mqtt_client_publish(mqtt_client, "Polytech/AREA1/S113/humidity", hum_1_data, 0, 1, 0);
  int msg_id3_2 = esp_mqtt_client_publish(mqtt_client, "Polytech/AREA1/S218/temerature", temp_2_data, 0, 1, 0);
  int msg_id4_2 = esp_mqtt_client_publish(mqtt_client, "Polytech/AREA1/S218/humidity", hum_2_data, 0, 1, 0);

  if (msg_id1_2 != -1 || msg_id2_2 != -1 || msg_id3_2 != -1 ||msg_id4_2 != -1)
  {
    ESP_LOGI(TAG, "Published temp and hum to Sxxx_Room, msg_id1=%d, msg_id2=%d, msg_id3=%d, msg_id4=%d", msg_id1_2, msg_id2_2, msg_id3_2, msg_id4_2);
  }    

  else 
  {
    ESP_LOGE(TAG, "Failed to publish temp and humidity to Pub_Sxxx_Room");
  }


  /* Unsubscribe of the End-Node : Sub_AREA1 */
  // esp_mqtt_client_unsubscribe()
  int msg_id_area1_2 = esp_mqtt_client_unsubscribe(mqtt_client, "Polytech/AREA1/#");
  if (msg_id_area1_2 != -1)
  {
    ESP_LOGI(TAG, "unsubscribe from AREA1 successful, msg_id=%d", msg_id_area1_2);
  }

  else 
  {
    ESP_LOGE(TAG, "Failed to unsubscribe from AREA1");
  }


  /* Publishing temperature with the End-Node : Pub_E110_Room */
  int msg_id_e110_4 = esp_mqtt_client_publish(mqtt_client, "Polytech/AREA1/E110/temperature", temp_data, 0, 1, 0);
  if (msg_id_e110_4 != -1)
  {
    ESP_LOGI(TAG, "Sent publish temp E110 successful, msg_id=%d", msg_id_e110_4);
  }

  else 
  {
  ESP_LOGE(TAG, "Failed to send publish to Pub_E110");
  }
    
  /* Wait 3 sec. */
  vTaskDelay(pdMS_TO_TICKS(3000));

  /* Publishing humidity with the End-Node : Pub_Sxxx_Room */

  int msg_id2_3 = esp_mqtt_client_publish(mqtt_client, "Polytech/AREA1/S113/humidity", hum_1_data, 0, 1, 0);
  int msg_id4_3 = esp_mqtt_client_publish(mqtt_client, "Polytech/AREA1/S218/humidity", hum_2_data, 0, 1, 0);

  if (msg_id2_3 != -1 || msg_id4_3 != -1)
  {
    ESP_LOGI(TAG, "Published hum to Sxxx_Room, msg_id2=%d, msg_id4=%d", msg_id2_3, msg_id4_3);
  }    

  else 
  {
    ESP_LOGE(TAG, "Failed to publish humidity to Pub_Sxxx_Room");
  }


  vTaskDelete(NULL);
}


void app_main() {
  //ERROR, WARNING, INFO level log 
  esp_log_level_set(TAG, ESP_LOG_INFO);

  //Init WiFi 
  wifiInit();
 
  //Create connected WiFi Task, STACK=3*1024, Priority=5
  xTaskCreate(connectedWifiTask, "connectedWifiTask", 3*1024, (void*)"connectedWifiTask", 6, NULL);

  //Delete task 
  vTaskDelete(NULL);
}