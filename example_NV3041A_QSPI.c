#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_nv3041a.h"
#include "driver/spi_master.h"

#define EXAMPLE_LCD_HOST SPI2_HOST
#define EXAMPLE_PIN_NUM_LCD_CS   5
#define EXAMPLE_PIN_NUM_LCD_DC   6
#define EXAMPLE_PIN_NUM_LCD_RST  7
#define EXAMPLE_PIN_NUM_LCD_SCLK 8
#define EXAMPLE_PIN_NUM_LCD_D0   9   // MOSI
#define EXAMPLE_PIN_NUM_LCD_D1   10  // MISO
#define EXAMPLE_PIN_NUM_LCD_D2   11  // WP
#define EXAMPLE_PIN_NUM_LCD_D3   12  // HD
#define EXAMPLE_LCD_H_RES 480
#define EXAMPLE_LCD_V_RES 272

void app_main(void)
{
    ESP_LOGI(TAG, "Initialize QSPI bus");
    const spi_bus_config_t bus_config = 
        NV3041A_PANEL_BUS_QSPI_CONFIG(
            EXAMPLE_PIN_NUM_LCD_SCLK,
            EXAMPLE_PIN_NUM_LCD_D0,
            EXAMPLE_PIN_NUM_LCD_D1,
            EXAMPLE_PIN_NUM_LCD_D2,
            EXAMPLE_PIN_NUM_LCD_D3,
            EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(uint16_t)
        );
    ESP_ERROR_CHECK(spi_bus_initialize(EXAMPLE_LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install QSPI panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    const esp_lcd_panel_io_spi_config_t io_config = 
        NV3041A_PANEL_IO_QSPI_CONFIG(
            EXAMPLE_PIN_NUM_LCD_CS,
            EXAMPLE_PIN_NUM_LCD_DC,
            NULL,  // callback
            NULL   // callback context
        );
    
    // Используем стандартную функцию, она поддерживает quad_mode через флаги
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)EXAMPLE_LCD_HOST,
        &io_config,
        &io_handle
    ));

    ESP_LOGI(TAG, "Install NV3041A panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_nv3041a(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    
    ESP_LOGI(TAG, "NV3041A QSPI display initialized!");
}