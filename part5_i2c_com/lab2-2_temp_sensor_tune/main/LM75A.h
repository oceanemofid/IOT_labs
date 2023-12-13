/****************************************************************************
 * Copyright (C) 2021 by Fabrice Muller                                     *
 *                                                                          *
 * This file is useful for ESP32 Design course.                             *
 *                                                                          *
 ****************************************************************************/

/**
 * @file LM75A.h
 * @author Fabrice Muller
 * @date 12 Nov. 2021
 * @brief File containing the lab2-1 of Part 5.
 *
 * @see https://github.com/fmuller-pns/esp32-vscode-project-template
 */

#ifndef _LM75A_H_
#define _LM75A_H_

#include <stdio.h>
#include "driver/i2c.h"

// slave address
#define LM75A_ADDRESS 0x48

// Registers
#define TEMP_REG_OFFSET 0x0
#define CONF_REG_OFFSET 0x1
#define THYST_REG_OFFSET 0x2
#define TOS_REG_OFFSET 0x3

void lm75a_init(uint8_t i2c_port_, uint8_t i2c_addr_);

esp_err_t lm75a_readRegister(uint8_t *raw);

esp_err_t lm75a_getTemperatureInDegree(float *temperature);
float convertRawToTemperature(uint8_t *raw);

/**
 * @brief Write to the configuration register (1 byte)
 *
 * @param value to be written
 * @return esp_err_t
 */
esp_err_t lm75a_writeConfigRegister(uint8_t value);

/**
 * @brief Convert the temperature to raw (useful for TOS and THYST registers)
 *
 * @param temp temperature
 * @param raw array of 2 bytes computed from the temperature field
 */
void convertTemperatureToRaw(float temp, uint8_t *raw);

/**
 * @brief Read raw (2 bytes) register for a addr offset
 *
 * @param offset_addr offset register
 * @param raw result value
 * @return esp_err_t ESP-OK if no error
 */
esp_err_t lm75a_readRegisterWithPointer(uint8_t offset_addr, uint8_t *raw);

/**
 * @brief Write to the TOS or THYS register a raw value (2 bytes)
 *
 * @param offset_addr offset of the register
 * @param raw raw (array of 2 bytes) to be written
 * @return esp_err_t
 */
esp_err_t lm75a_writeThysOrTosRegister(uint8_t offset_addr, uint8_t *raw);

#endif
