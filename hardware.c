#include "hardware.h"

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <driver/i2c.h>
#include <esp_log.h>

#include "rp2040.h"

static const char* TAG = "hardware";

static ICE40   dev_ice40   = {0};
static RP2040  dev_rp2040  = {0};

static uint8_t rp2040_fw_version = 0;

static bool bsp_ready    = false;
static bool rp2040_ready = false;
static bool ice40_ready  = false;

xSemaphoreHandle i2c_semaphore = NULL;

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

static esp_err_t _bus_init() {
    esp_err_t res;

    // I2C bus
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_I2C_SDA,
        .scl_io_num = GPIO_I2C_SCL,
        .master.clk_speed = I2C_SPEED,
        .sda_pullup_en = false,
        .scl_pullup_en = false,
        .clk_flags = 0
    };

    res = i2c_param_config(I2C_BUS, &i2c_config);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Configuring I2C bus parameters failed");
        return res;
    }

    res = i2c_set_timeout(I2C_BUS, I2C_TIMEOUT * 80);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Configuring I2C bus timeout failed");
        return res;
    }

    res = i2c_driver_install(I2C_BUS, i2c_config.mode, 0, 0, 0);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing system I2C bus failed");
        return res;
    }

    i2c_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive( i2c_semaphore );

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

    // LCD to FPGA
    res = gpio_set_direction(GPIO_LCD_MODE, GPIO_MODE_OUTPUT);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing LCD mode GPIO failed");
        return res;
    }

    res = gpio_set_level(GPIO_LCD_MODE, true);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Setting LCD mode failed");
    }

    bsp_ready = true;
    return ESP_OK;
}

esp_err_t bsp_rp2040_init() {
    if (!bsp_ready) return ESP_FAIL;
    if (rp2040_ready) return ESP_OK;

    // RP2040 co-processor
    dev_rp2040.i2c_bus       = I2C_BUS;
    dev_rp2040.i2c_address   = RP2040_ADDR;
    dev_rp2040.pin_interrupt = GPIO_INT_RP2040;
    dev_rp2040.queue         = xQueueCreate(8, sizeof(rp2040_input_message_t));
    dev_rp2040.i2c_semaphore = i2c_semaphore;

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
    dev_ice40.spi_input_delay_ns    = 10;
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

RP2040* get_rp2040() {
    if (!rp2040_ready) return NULL;
    return &dev_rp2040;
}

ICE40* get_ice40() {
    if (!ice40_ready) return NULL;
    return &dev_ice40;
}
