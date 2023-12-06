/****************************************************************************
 * Copyright (C) 2021 by Fabrice Muller                                     *
 *                                                                          *
 * This file is useful for ESP32 Design course.                             *
 *                                                                          *
 ****************************************************************************/

/**
 * @file main.c
 * @author Fabrice Muller
 * @date 08 Nov. 2021
 * @brief File containing the lab4 of Part 4.
 *
 * @see https://github.com/fmuller-pns/esp32-vscode-project-template
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"

#include "esp_intr_alloc.h"

#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "my_helper_fct.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "driver/adc.h"
#include "driver/adc_common.h"
#include "esp_adc_cal.h"

#include "driver/ledc.h"
#include "soc/ledc_reg.h"

static const char *TAG = "MAIN";

/******************** ADC Declaration **********************/

// Vref in mV
#define DEFAULT_VREF 1100
const adc_unit_t unit = ADC_UNIT_1;
const adc_channel_t channel = ADC_CHANNEL_1;
const adc_bits_width_t width = ADC_WIDTH_BIT_10;
const adc_atten_t atten = ADC_ATTEN_DB_11;

esp_adc_cal_characteristics_t *adc_chars;

/******************** UART Declaration **********************/
// GPIO18 / IO18
#define TXD_PIN 18
// GPIO23 / IO23
#define RXD_PIN 23
// Not Connected
#define RTS_PIN (UART_PIN_NO_CHANGE)
// Not Connected
#define CTS_PIN (UART_PIN_NO_CHANGE)

#define UART_PORT_NUM 2
#define UART_BAUD_RATE 115200

QueueHandle_t uart_queue;

#define BUF_SIZE (1024)
static char data[BUF_SIZE];

/******************** Task Declaration **********************/

/* The tasks */
static const uint32_t STACK_SIZE = 4000;
static const uint32_t UpdateLedTask_PRIO = 2;
static const uint32_t ScanTask_PRIO = 2;

void vUpdateLedTask(void *pvParameters);
void vScanTask(void *pvParameters);

/* LED constants */
static const char *WHITE_LED_CMD = "WHITE";
static const char *BLUE_LED_CMD = "BLUE";

/**
 * @brief Starting point function
 *
 */

void app_main(void)
{

	printf("Application - Photoresistance - IHM Node-RED\n");

	/* Configure parameters of the UART driver,
	 * communication pins and install the driver.
	 *
	 * Configuration: 115000 BAUDS, 8 BITS, No Parity, 1 STOP BIT, No Flow Control, APB CLK
	 */
	uart_config_t uart_config = {
		.baud_rate = UART_BAUD_RATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_APB,
	};

	ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart_queue, 0));
	ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
	ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, RTS_PIN, CTS_PIN));

	/* Configure ADC : ADC1, Channel 1, 10 bits, Attenuation 11dB */
	adc1_config_width(width);	
   	adc1_config_channel_atten(channel, atten);

	/* Characterize ADC */
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);

	/*
	 * LEDC Timer configuration
	 * Timer 0, Low speed mode, Auto clk, Résolution 10bits, Frequency 5 KHz
	 */
	ledc_timer_config_t timer = {
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.duty_resolution = LEDC_TIMER_10_BIT,
		.timer_num = LEDC_TIMER_0,
		.freq_hz = 5000,
		.clk_cfg = LEDC_AUTO_CLK};

	ledc_timer_config(&timer);

	/*
	 * Channel 0 configuration
	 * Timer 0, Channel 0, GPIO21 pin, Low speed mode
	 */
	ledc_channel_config_t channel_0 = {
		.gpio_num = 21,
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.channel = LEDC_CHANNEL_0,
		.timer_sel = LEDC_TIMER_0,
		.duty = 0,
		.hpoint = 0 };
	ledc_channel_config(&channel_0);

	/*
	 * Channel 1 configuration
	 * Timer 0, Channel 1, GPIO22 pin, Low speed mode
	 */
	ledc_channel_config_t channel_1 = {
		.gpio_num = 22,
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.channel = LEDC_CHANNEL_1,
		.timer_sel = LEDC_TIMER_0,
		.duty = 0,
		.hpoint = 0};
	ledc_channel_config(&channel_1);

	esp_err_t result = ledc_fade_func_install(0); // 0 : no interrupt
	if (result != ESP_OK)
	{
		printf("Error installing fade: %04x\n", result);
		return;
	}


	vTaskSuspendAll();

	/* Create Tasks */
	xTaskCreatePinnedToCore(vUpdateLedTask, "vUpdateLedTask", STACK_SIZE, (void *)"vUpdateLedTask ", UpdateLedTask_PRIO, NULL, CORE_1);
	xTaskCreatePinnedToCore(vScanTask, "vScanTask", STACK_SIZE, (void *)"vScanTask ", ScanTask_PRIO, NULL, CORE_0);

	/* Continue scheduler */
	xTaskResumeAll();

	/* Delete Main task */
	vTaskDelete(NULL);
}

void vUpdateLedTask(void *pvParameters)
{

	uart_event_t event;

	for (;;)
	{
		/* Wait for message with infinite timeout */
		if (xQueueReceive(uart_queue, (void *)&event, (portTickType)portMAX_DELAY))
		{

			switch (event.type)
			{
			// Event of received data
			case UART_DATA:
				uart_read_bytes(UART_PORT_NUM, data, event.size, portMAX_DELAY);
				DISPLAY("[UART DATA]: %.*s", event.size, data);

				data[event.size] = 0; // Be sure the last character in NULL (end of string)
				int cmp_length;
				char *ptr;

				/* Extract Value */
				char *str_value = strrchr(data, ':');
				if (str_value != NULL)
					cmp_length = event.size - strlen(str_value);
				else
				{
					DISPLAYE(TAG, "[UART DATA]: Format Error");
					continue;
				}
				uint32_t value = strtoul((str_value + 1), &ptr, 10);

				/* duty cycle according to the color LED */
				if (strncmp(WHITE_LED_CMD, data, cmp_length) == 0)
				{
					ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,value,0);
					ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_1,0,0);
					DISPLAY("White LED duty set to %d", value);
				}
				else if (strncmp(BLUE_LED_CMD, data, cmp_length) == 0)
				{
					ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_1,value,0);
					ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,0,0);
					DISPLAY("Blue LED duty set to %d", value);
				}
				break;

			// Event of HW FIFO overflow detected
			case UART_FIFO_OVF:
				DISPLAYE(TAG, "hw fifo overflow");
				uart_flush_input(UART_PORT_NUM);
				xQueueReset(uart_queue);
				break;

			// Event of UART ring buffer full
			case UART_BUFFER_FULL:
				ESP_LOGI(TAG, "ring buffer full");
				uart_flush_input(UART_PORT_NUM);
				xQueueReset(uart_queue);
				break;

			// Event of UART RX break detected
			case UART_BREAK:
				DISPLAYE(TAG, "uart rx break");
				break;
			// Event of UART parity check error
			case UART_PARITY_ERR:
				DISPLAYE(TAG, "uart parity error");
				break;
			// Event of UART frame error
			case UART_FRAME_ERR:
				DISPLAYE(TAG, "uart frame error");
				break;
			// UART_PATTERN_DET
			case UART_PATTERN_DET:
				DISPLAYW(TAG, "uart pattern detected");
				break;
			// Others
			default:
				DISPLAYW(TAG, "uart event type: %d", event.type);
				break;
			}
		}
	}

	vTaskDelete(NULL);
}


void vScanTask(void *pvParameters){
	DISPLAY("Update Scan Task");
	uint16_t measuredValues;
	int v;
	char text[100];
	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();

	while(1)
	{
		v = adc1_get_raw(channel);
		measuredValues = esp_adc_cal_raw_to_voltage(v,adc_chars);
		vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(1000));

		DISPLAY("measuredValue : %d",measuredValues);
		sprintf(text, "%d\n\r",measuredValues);

		// Write data back to the UART
 		int length = uart_write_bytes(UART_PORT_NUM, (const char *) text, strlen(text));
        if (length == -1)
            ESP_LOGE(TAG, "Error when writing in UART.");

		xLastWakeTime = xTaskGetTickCount();
	}

}