#pragma once

#include <driver/spi_master.h>
#include <esp_err.h>
#include <sdkconfig.h>

#include "bno055.h"
#include "ice40.h"
#include "ili9341.h"
#include "mch2022_badge.h"
#include "rp2040.h"
#include "bme680.h"

/** \brief Initialize basic board support
 *
 * \details This function installs the GPIO ISR (interrupt service routine) service
 *          which allows for per-pin GPIO interrupt handlers. Then it initializes the
 *          I2C and SPI communication busses and the LCD display driver. Returns ESP_OK
 *          on success and a
 *
 * \retval ESP_OK   The function succesfully executed
 * \retval ESP_FAIL The function failed, possibly indicating hardware failure
 *
 * Check the esp_err header file from the ESP-IDF for a complete list of error codes
 * returned by SDK functions.
 */

esp_err_t bsp_init();

/** \brief Initialize RP2040 driver and check I2C communication with the RP2040 co-processor
 *
 * \details This function initializes the RP2040 driver, which installs the RP2040 interrupt
 *          handler task. Then it queries the version of the firmware running on the RP2040.
 *          Reading the firmware version verifies that communication via I2C is funcitonal and
 *          the firmware version indicates weither or not the RP2040 is in bootloader
 *          mode. If the RP2040 is in bootloader mode it reports version 0xFF.
 *
 * \retval ESP_OK   The function succesfully executed
 * \retval ESP_FAIL The function failed, possibly indicating hardware failure
 *
 * Check the esp_err header file from the ESP-IDF for a complete list of error codes
 * returned by SDK functions.
 */

esp_err_t bsp_rp2040_init();

/** \brief Initialize the ICE40 driver and perform basic sanity checks
 *
 * \details This function initializes the ICE40 driver and checks that the status
 *          indication (the "done" pin) correctly indicates that the ICE40 is disabled.
 *
 * \retval ESP_OK   The function succesfully executed
 * \retval ESP_FAIL The function failed, possibly indicating hardware failure
 *
 * Check the esp_err header file from the ESP-IDF for a complete list of error codes
 * returned by SDK functions.
 */

esp_err_t bsp_ice40_init();

/** \brief Initialize the BNO055 driver and put the sensor into power saving mode
 *
 * \details This function initializes the BNO055 driver and puts the sensor into
 *          power saving mode.
 *
 * \retval ESP_OK   The function succesfully executed
 * \retval ESP_FAIL The function failed, possibly indicating hardware failure
 *
 * Check the esp_err header file from the ESP-IDF for a complete list of error codes
 * returned by SDK functions.
 */

esp_err_t bsp_bno055_init();

/** \brief Initialize the BME680 driver and put the sensor into power saving mode
 *
 * \details This function initializes the BME680 driver and puts the sensor into
 *          power saving mode.
 *
 * \retval ESP_OK   The function succesfully executed
 * \retval ESP_FAIL The function failed, possibly indicating hardware failure
 *
 * Check the esp_err header file from the ESP-IDF for a complete list of error codes
 * returned by SDK functions.
 */

esp_err_t bsp_bme680_init();

/** \brief Fetch a handle for the ILI9341 LCD display hardware component
 *
 * \details This function returns a handle using which the ILI9341 driver can
 *          be controlled or NULL if the ILI9341 is not available.
 *
 * \retval struct:ILI9341 Structure describing the ILI9341 device, used to control the driver
 * \retval NULL           Device not available
 */

ILI9341* get_ili9341();

/** \brief Fetch a handle for the RP2040 co-processor hardware component
 *
 * \details This function returns a handle using which the RP2040 driver can
 *          be controlled or NULL if the RP2040 is not available.
 *
 * \retval struct:ILI9341 Structure describing the RP2040 device, used to control the driver
 * \retval NULL           Device not available
 */

RP2040* get_rp2040();

/** \brief Fetch a handle for the ICE40 FPGA hardware component
 *
 * \details This function returns a handle using which the ICE40 driver can
 *          be controlled or NULL if the ICE40 is not available.
 *
 * \retval struct:ILI9341 Structure describing the ICE40 device, used to control the driver
 * \retval NULL           Device not available
 */

ICE40* get_ice40();

/** \brief Fetch a handle for the BNO055 sensor hardware component
 *
 * \details This function returns a handle using which the BNO055 driver can
 *          be controlled or NULL if the BNO055 is not available.
 *
 * \retval struct:ILI9341 Structure describing the BNO055 device, used to control the driver
 * \retval NULL           Device not available
 */

BNO055* get_bno055();

/** \brief Fetch a handle for the BME680 sensor hardware component
 *
 * \details This function returns a handle using which the BME680 driver can
 *          be controlled or NULL if the BME680 is not available.
 *
 * \retval struct:ILI9341 Structure describing the BME680 device, used to control the driver
 * \retval NULL           Device not available
 */

BNO055* get_bme680();
