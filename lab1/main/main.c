/****************************************************************************
 * Copyright (C) 2021 by Fabrice Muller                                     *
 *                                                                          *
 * This file is useful for ESP32 Design course.                             *
 *                                                                          *
 ****************************************************************************/

/**
 * @file main.c
 * @author Fabrice Muller
 * @date 31 Oct. 2021
 * @brief File containing the lab1 of Part 4.
 *
 * @see https://github.com/fmuller-pns/esp32-vscode-project-template
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FreeRTOS.org includes. */
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_timer.h"
#include "esp_sleep.h"

#include "driver/gpio.h"
#include "driver/dac.h"

int N=0;
int led_flag=0;
esp_timer_handle_t esp_timer_handle1;



void triangle_cb(void* arg)
{
    if(led_flag==0)
    {
        if(N<250)
            ++N;
        if(N==250)
            led_flag=1;
    }
    else
    {
        if(N>0)
            --N;
        if(N==0)
            led_flag=0;
    }

    //call the dac_output_voltage function to output the value of N to DAC_CHANNEL_1
    dac_output_voltage(DAC_CHANNEL_1,N);
}



void app_main(void)
{

    /**************************************************/
    /* Configure DAC (Digital to Analog Converter)    */

    /* DAC_CHANNEL_1 = GPIO25 (IO25) */
    dac_output_enable(DAC_CHANNEL_1);

    /* Set Value */
    dac_output_voltage(DAC_CHANNEL_1, 0);

    /**************************************************/
    /* Configure Timer                                */
    const esp_timer_create_args_t esp_timer_create_args1 = {
        .callback = triangle_cb,
        .name = "MyFctName"};

  
    //esp_timer_handle_t esp_timer_handle;

    //creat timer
    esp_timer_create(&esp_timer_create_args1, &esp_timer_handle1);
    

    /* Start timer  */
    esp_timer_start_periodic(esp_timer_handle1, 10000);
    vTaskDelay(pdMS_TO_TICKS(15000));
    

    /* Display timer information */
    esp_timer_dump(stdout);

    /* Stop Timer */
    esp_timer_stop(esp_timer_handle1);
    

    /* Re-Start timer  */

    esp_timer_start_periodic(esp_timer_handle1, 1000);
    vTaskDelay(pdMS_TO_TICKS(15000));
    esp_timer_stop(esp_timer_handle1);

    /* Delete Timer */
    esp_timer_delete(esp_timer_handle1);


    /* to ensure its exit is clean */
    vTaskDelete(NULL);
}
