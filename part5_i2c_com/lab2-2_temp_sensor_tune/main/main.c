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
 * @brief File containing the lab2-1 of Part 5.
 *
 * @see https://github.com/fmuller-pns/esp32-vscode-project-template
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_types.h"
#include "esp_log.h"
#include "esp_freertos_hooks.h"
#include "my_helper_fct.h"


#include "LM75A.h"

static const char *TAG = "MAIN";

/* Must be added on declaration part */
#define STACK_SIZE 4096

static const float MIN_OFFSET_TEMP = 0.6F; 
static const float MAX_OFFSET_TEMP = 0.6F; 

static float currentTosTemperature;
static float currentThystTemperature;

/*
 * I2C Pins
 * SDA: GPIO26, SCL: GPIO25
 */
#define SDA_GPIO 26
#define SCL_GPIO 25

// I2C port number : N°0
#define LM75A_PORT_NUM 0

/**
 * @brief The task reads Temperature every 2 seconds.
 * 
 * @param pvParameters 
 */
void vTaskControlTemperature(void *pvParameters) {
    float temperature;
	esp_err_t result;
	uint8_t raw[2];


    for (;;) {
        /* Period = 2 sec. */
		vTaskDelay(pdMS_TO_TICKS(2000));

        /* Read Temperature register */
		if ((result = lm75a_readRegisterWithPointer(TEMP_REG_OFFSET, raw)) != ESP_OK)
            ESP_LOGE(TAG, "Error while reading TEMP reg.");

        /* Compute temperature */
		temperature = convertRawToTemperature(raw);
        printf("Temperature: %.2f °C\n", temperature);

        /* Display Information */
        if (temperature > currentTosTemperature) {
            DISPLAY("Temperature: %.2f°C > TOS (%.2f°C)", temperature, currentTosTemperature);
        }
        else if (temperature < currentThystTemperature) {
            DISPLAY("Temperature: %.2f°C < THYST (%.2f°C)", temperature, currentThystTemperature);
        }
        else {
            DISPLAY("Temperature: THYST (%.2f°C) < %.2f °C < TOS (%.2f°C)", currentThystTemperature, temperature, currentTosTemperature);                
        }
    }
}

/**
 * @brief Starting point function
 * 
 */

void app_main(void)
{

	/* Install I2C driver : MASTER mode, GPIO SDA/SCL, SDA/SCL Pullup enable (GPIO_PULLUP ... constant), Freq=10KHz */
	i2c_config_t i2c_config = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = SDA_GPIO,
		.scl_io_num = SCL_GPIO,
		.sda_pullup_en = true,
		.scl_pullup_en = true,
		.master.clk_speed = 10000};
	i2c_param_config(LM75A_PORT_NUM, &i2c_config);
	i2c_driver_install(LM75A_PORT_NUM, I2C_MODE_MASTER, 0, 0, 0);

	/* Init LM75a for i2c */
	lm75a_init(LM75A_PORT_NUM, LM75A_ADDRESS);

	esp_err_t result;
	uint8_t raw[2];
	float temperature = 0;

	/*****************************************************************************
     * Init Configuration register for LM75A
     * 
     */

    // comparator mode + Active low (test active high also)
    uint8_t cfgValue = 0x00;
	raw[1]=0;
    raw[0]=0;
    if ((result = lm75a_writeConfigRegister(cfgValue)) != ESP_OK)
        ESP_LOGE(TAG, "Error while writing conf reg.");

    if ((result = lm75a_readRegisterWithPointer(CONF_REG_OFFSET, raw)) != ESP_OK)
        ESP_LOGE(TAG, "Error while reading TEMP reg.");


    /*****************************************************************************
     * Adjust Tos and Thyst registers
     * Tos = current temperature - MIN_OFFSET_TEMP
     * Thyst = current temperature + MAX_OFFSET_TEMP
     */

	/* Read TEMP Reg. */
    if ((result = lm75a_readRegisterWithPointer(TEMP_REG_OFFSET, raw)) != ESP_OK)
        ESP_LOGE(TAG, "Error while reading TEMP reg.");

    /* Compute temperature */
    temperature = convertRawToTemperature(raw);
    printf("Temperature: %.2f °C\n", temperature);

    /* Write Thyst register */
    currentThystTemperature = temperature - MIN_OFFSET_TEMP;
    convertTemperatureToRaw(currentThystTemperature, raw);
    if ((result = lm75a_writeThysOrTosRegister(THYST_REG_OFFSET, raw)) != ESP_OK)
        ESP_LOGE(TAG, "Error while writing THYST reg.");

    /* Write Tos register */
    currentTosTemperature = temperature + MAX_OFFSET_TEMP;
    convertTemperatureToRaw(currentTosTemperature, raw);
    if ((result = lm75a_writeThysOrTosRegister(TOS_REG_OFFSET, raw)) != ESP_OK)
        ESP_LOGE(TAG, "Error while writing TOS reg.");

	    /*****************************************************************************
     * Check Tos and Thyst registers
     * 
     */

    /* Read Thyst register */
    if ((result = lm75a_readRegisterWithPointer(THYST_REG_OFFSET, raw)) != ESP_OK)
        ESP_LOGE(TAG, "Error while reading THYST reg.");

    // Compute THYS temperature
    temperature = convertRawToTemperature(raw);
    printf("THYS Temperature: %.2f °C\n", temperature);

    /* Read TOS register */
    if ((result = lm75a_readRegisterWithPointer(TOS_REG_OFFSET, raw)) != ESP_OK)
        ESP_LOGE(TAG, "Error while reading TOS reg.");

    // Compute TOS temperature
    temperature = convertRawToTemperature(raw);
    printf("TOS Temperature: %.2f °C\n", temperature);

    /* Read TEMP for update Pointer Reg. = offset 0 */
    if ((result = lm75a_readRegisterWithPointer(TEMP_REG_OFFSET, raw)) != ESP_OK)
        ESP_LOGE(TAG, "Error while reading TEMP reg.");

    /******************************************************************
     * Create application
     * 
     */

	/* Create Task: vTaskControlTemperature on Core 0 */
	xTaskCreatePinnedToCore(vTaskControlTemperature,"controlTemp",STACK_SIZE,(void*)"vTaskControlTemperature",1,NULL,CORE_0);
    
    /* Delete Task */
    vTaskDelete(NULL);
}


