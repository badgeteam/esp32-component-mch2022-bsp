#include "hardware.h"

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>

#include "managed_i2c.h"
#include "rp2040.h"
#include "sdcard.h"

static const char* TAG = "hardware";

static BNO055  dev_bno055  = {0};
static ILI9341 dev_ili9341 = {0};
static ICE40   dev_ice40   = {0};
static RP2040  dev_rp2040  = {0};
static BME680  dev_bme680  = {0};

static uint8_t rp2040_fw_version = 0;

static bool bsp_ready    = false;
static bool rp2040_ready = false;
static bool ice40_ready  = false;
static bool bno055_ready = false;
static bool bme680_ready = false;

esp_err_t ice40_get_done_wrapper(bool* done) {
    uint16_t  buttons;
    esp_err_t res = rp2040_read_buttons(&dev_rp2040, &buttons);
    if (res != ESP_OK) return res;
    *done = !((buttons >> 5) & 0x01);
    return ESP_OK;
}

esp_err_t ice40_set_reset_wrapper(bool reset) {
    esp_err_t res = rp2040_set_fpga(&dev_rp2040, reset);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    return res;
}

void ili9341_set_lcd_mode(bool mode) {
    ESP_LOGI(TAG, "LCD mode switch to %s", mode ? "FPGA" : "ESP32");
    esp_err_t res = gpio_set_level(GPIO_LCD_MODE, mode);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Setting LCD mode failed");
    }
}

static esp_err_t _bus_init() {
    esp_err_t res;

    // I2C bus
    res = i2c_init(I2C_BUS_SYS, GPIO_I2C_SYS_SDA, GPIO_I2C_SYS_SCL, I2C_SPEED_SYS, false, false);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing system I2C bus failed");
        return res;
    }

    // SPI bus
    spi_bus_config_t busConfiguration = {0};
    busConfiguration.mosi_io_num      = GPIO_SPI_MOSI;
    busConfiguration.miso_io_num      = GPIO_SPI_MISO;
    busConfiguration.sclk_io_num      = GPIO_SPI_CLK;
    busConfiguration.quadwp_io_num    = -1;
    busConfiguration.quadhd_io_num    = -1;
    busConfiguration.max_transfer_sz  = SPI_MAX_TRANSFER_SIZE;
    res                               = spi_bus_initialize(SPI_BUS, &busConfiguration, SPI_DMA_CHANNEL);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing SPI bus failed");
        return res;
    }

    return ESP_OK;
}

esp_err_t bsp_init() {
    if (bsp_ready) return ESP_OK;

    esp_err_t res;

    // Interrupts
    res = gpio_install_isr_service(0);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Installing ISR service failed");
        return res;
    }

    // Communication busses
    res = _bus_init();
    if (res != ESP_OK) return res;

    // LCD display
    dev_ili9341.spi_bus               = SPI_BUS;
    dev_ili9341.pin_cs                = GPIO_SPI_CS_LCD;
    dev_ili9341.pin_dcx               = GPIO_SPI_DC_LCD;
    dev_ili9341.pin_reset             = GPIO_LCD_RESET;
    dev_ili9341.rotation              = 1;
    dev_ili9341.color_mode            = true;      // Blue and red channels are swapped
    dev_ili9341.spi_speed             = 40000000;  // 40MHz
    dev_ili9341.spi_max_transfer_size = SPI_MAX_TRANSFER_SIZE;
    dev_ili9341.callback              = ili9341_set_lcd_mode;  // Callback for changing LCD mode between ESP32 and FPGA

    res = gpio_set_direction(GPIO_LCD_MODE, GPIO_MODE_OUTPUT);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing LCD mode GPIO failed");
        return res;
    }

    res = ili9341_init(&dev_ili9341);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing LCD failed");
        return res;
    }

    bsp_ready = true;
    return ESP_OK;
}

esp_err_t bsp_rp2040_init() {
    if (!bsp_ready) return ESP_FAIL;
    if (rp2040_ready) return ESP_OK;

    // RP2040 co-processor
    dev_rp2040.i2c_bus       = I2C_BUS_SYS;
    dev_rp2040.i2c_address   = RP2040_ADDR;
    dev_rp2040.pin_interrupt = GPIO_INT_RP2040;
    dev_rp2040.queue         = xQueueCreate(8, sizeof(rp2040_input_message_t));

    esp_err_t res = rp2040_init(&dev_rp2040);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing RP2040 failed");
        return res;
    }

    if (rp2040_get_firmware_version(&dev_rp2040, &rp2040_fw_version) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read RP2040 firmware version");
        return ESP_FAIL;
    }

    rp2040_ready = true;
    return ESP_OK;
}

esp_err_t bsp_ice40_init() {
    if (!bsp_ready) return ESP_FAIL;
    if (!rp2040_ready) return ESP_FAIL;
    if (rp2040_fw_version == 0xFF) return ESP_FAIL;  // The ICE40 FPGA can only be controlled when the RP2040 is not in bootloader mode
    if (ice40_ready) return ESP_OK;

    dev_ice40.spi_bus               = SPI_BUS;
    dev_ice40.pin_cs                = GPIO_SPI_CS_FPGA;
    dev_ice40.pin_done              = -1;
    dev_ice40.pin_reset             = -1;
    dev_ice40.pin_int               = GPIO_INT_FPGA;
    dev_ice40.spi_speed_full_duplex = 26700000;
    dev_ice40.spi_speed_half_duplex = 40000000;
    dev_ice40.spi_speed_turbo       = 80000000;
    dev_ice40.spi_input_delay_ns    = 15;
    dev_ice40.spi_max_transfer_size = SPI_MAX_TRANSFER_SIZE;
    dev_ice40.get_done              = ice40_get_done_wrapper;
    dev_ice40.set_reset             = ice40_set_reset_wrapper;

    esp_err_t res = ice40_init(&dev_ice40);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing ICE40 failed");
        return res;
    }

    bool done;
    res = ice40_get_done(&dev_ice40, &done);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ICE40 done state");
        return res;
    }

    if (done) {
        ESP_LOGE(TAG, "ICE40 indicates done in disabled state");
        return ESP_FAIL;
    }

    ice40_ready = true;
    return ESP_OK;
}

esp_err_t bsp_bno055_init() {
    if (!bsp_ready) return ESP_FAIL;
    if (bno055_ready) return ESP_OK;

    esp_err_t res = bno055_init(&dev_bno055, I2C_BUS_SYS, BNO055_ADDR, GPIO_INT_BNO055, true);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing BNO055 failed");
        return res;
    }

    res = bno055_set_power_mode(&dev_bno055, BNO055_POWER_MODE_SUSPEND);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to switch BNO055 power mode to suspended state");
        return res;
    }

    bno055_ready = true;
    return ESP_OK;
}

esp_err_t bsp_bme680_init() {
    if (!bsp_ready) return ESP_FAIL;
    if (bme680_ready) return ESP_OK;

    dev_bme680.i2c_bus = I2C_BUS_SYS;
    dev_bme680.i2c_address = BME680_ADDR;

    esp_err_t res = bme680_init(&dev_bme680);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing BME680 failed");
        return res;
    }

    bme680_ready = true;
    return ESP_OK;
}

ILI9341* get_ili9341() {
    if (!bsp_ready) return NULL;
    return &dev_ili9341;
}

RP2040* get_rp2040() {
    if (!rp2040_ready) return NULL;
    return &dev_rp2040;
}

ICE40* get_ice40() {
    if (!ice40_ready) return NULL;
    return &dev_ice40;
}

BNO055* get_bno055() {
    if (!bno055_ready) return NULL;
    return &dev_bno055;
}

BME680* get_bme680() {
    if (!bme680_ready) return NULL;
    return &dev_bme680;
}
