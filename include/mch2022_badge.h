/*
 * MCH2022 badge
 *
 * This file describes the connections from the ESP32 to the other
 * components on the MCH2022 badge.
 *
 * More information can be found in the documentation at
 * https://www.badge.team/docs/badges/mch2022/pinout
 *
 */

#pragma once

#define GPIO_I2S_MCLK    0
#define GPIO_UART_TX     1
#define GPIO_SD_D0       2
#define GPIO_UART_RX     3
#define GPIO_I2S_LR      4
#define GPIO_LED_DATA    5
#define GPIO_I2S_CLK     12
#define GPIO_I2S_DATA    13
#define GPIO_SD_CLK      14
#define GPIO_SD_CMD      15
#define GPIO_SPI_CLK     18
#define GPIO_SD_PWR      19  // Also LED power
#define GPIO_I2C_SCL     21
#define GPIO_I2C_SDA     22
#define GPIO_SPI_MOSI    23
#define GPIO_LCD_RESET   25
#define GPIO_LCD_MODE    26
#define GPIO_SPI_CS_FPGA 27
#define GPIO_SPI_CS_LCD  32
#define GPIO_SPI_DC_LCD  33
#define GPIO_INT_RP2040  34  // Active low
#define GPIO_SPI_MISO    35
#define GPIO_INT_BNO055  36  // Active low
#define GPIO_INT_FPGA    39  // Active low

// I2C bus
#define I2C_BUS        0
#define I2C_SPEED      400000  // 400 kHz
#define I2C_TIMEOUT    250 // us

#define RP2040_ADDR 0x17  // RP2040 co-processor
#define BNO055_ADDR 0x28  // BNO055 position sensor
#define BME680_ADDR 0x77  // BME680 environmental sensor

// SPI bus
#define SPI_BUS               VSPI_HOST
#define SPI_MAX_TRANSFER_SIZE 4094
#define SPI_DMA_CHANNEL       2
