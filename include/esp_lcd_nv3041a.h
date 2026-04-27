/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_lcd_panel_vendor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Driver version
 */
#define ESP_LCD_NV3041A_VER_MAJOR 0
#define ESP_LCD_NV3041A_VER_MINOR 1
#define ESP_LCD_NV3041A_VER_PATCH 2

/**
 * @brief LCD panel initialization commands.
 */
typedef struct {
    int cmd;                /*!< The specific LCD command */
    const void *data;       /*!< Buffer that holds the command specific data */
    size_t data_bytes;      /*!< Size of `data` in memory, in bytes */
    unsigned int delay_ms;  /*!< Delay in milliseconds after this command */
} nv3041a_lcd_init_cmd_t;

/**
 * @brief LCD panel vendor configuration.
 *
 * @note  This structure needs to be passed to the `vendor_config` field in
 *        `esp_lcd_panel_dev_config_t`.
 */
typedef struct {
    const nv3041a_lcd_init_cmd_t *init_cmds; /*!< Initialization commands array, set to NULL for default. */
    uint16_t init_cmds_size;                /*!< Number of commands in above array */
} nv3041a_vendor_config_t;

/**
 * @brief Create LCD panel for model NV3041A
 *
 * @note Vendor specific initialization can be different between manufacturers,
 *       should consult the LCD supplier for initialization sequence code.
 *
 * @param[in] io LCD panel IO handle
 * @param[in] panel_dev_config General panel device configuration
 * @param[out] ret_panel Returned LCD panel handle
 * @return
 *          - ESP_ERR_INVALID_ARG   if parameter is invalid
 *          - ESP_ERR_NO_MEM        if out of memory
 *          - ESP_OK                on success
 */
esp_err_t esp_lcd_new_panel_nv3041a(const esp_lcd_panel_io_handle_t io,
                                  const esp_lcd_panel_dev_config_t *panel_dev_config,
                                  esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief LCD panel bus configuration structure
 *
 * @param[in] sclk SPI clock pin number
 * @param[in] mosi SPI MOSI pin number
 * @param[in] max_trans_sz Maximum transfer size in bytes
 */
#define NV3041A_PANEL_BUS_SPI_CONFIG(sclk, mosi, max_trans_sz)  \
    {                                                           \
        .sclk_io_num = sclk,                                    \
        .mosi_io_num = mosi,                                    \
        .miso_io_num = -1,                                      \
        .quadhd_io_num = -1,                                    \
        .quadwp_io_num = -1,                                    \
        .max_transfer_sz = max_trans_sz,                        \
    }

/**
 * @brief LCD panel IO configuration structure
 *
 * @param[in] cs SPI chip select pin number
 * @param[in] dc SPI data/command pin number
 * @param[in] cb Callback function when SPI transfer is done
 * @param[in] cb_ctx Callback function context
 */
#define NV3041A_PANEL_IO_SPI_CONFIG(cs, dc, callback, callback_ctx) \
    {                                                               \
        .cs_gpio_num = cs,                                          \
        .dc_gpio_num = dc,                                          \
        .spi_mode = 0,                                              \
        .pclk_hz = 32 * 1000 * 1000,                                \
        .trans_queue_depth = 10,                                    \
        .on_color_trans_done = callback,                            \
        .user_ctx = callback_ctx,                                   \
        .lcd_cmd_bits = 8,                                          \
        .lcd_param_bits = 8,                                        \
    }

/**
 * @brief LCD panel bus configuration structure for QSPI
 *
 * @param[in] sclk SPI clock pin number
 * @param[in] d0 SPI D0 (MOSI) pin number
 * @param[in] d1 SPI D1 (MISO) pin number  
 * @param[in] d2 SPI D2 pin number
 * @param[in] d3 SPI D3 pin number
 * @param[in] max_trans_sz Maximum transfer size in bytes
 */
#define NV3041A_PANEL_BUS_QSPI_CONFIG(sclk, d0, d1, d2, d3, max_trans_sz) \
    { \
        .sclk_io_num = sclk, \
        .mosi_io_num = d0, \
        .miso_io_num = d1, \
        .quadwp_io_num = d2, \
        .quadhd_io_num = d3, \
        .max_transfer_sz = max_trans_sz, \
    }

/**
 * @brief LCD panel IO configuration structure for QSPI
 *
 * @param[in] cs SPI chip select pin number
 * @param[in] dc SPI data/command pin number
 * @param[in] cb Callback function when SPI transfer is done
 * @param[in] cb_ctx Callback function context
 */
#define NV3041A_PANEL_IO_QSPI_CONFIG(cs, dc, callback, callback_ctx) \
    { \
        .cs_gpio_num = cs, \
        .dc_gpio_num = dc, \
        .spi_mode = 0, \
        .pclk_hz = 80 * 1000 * 1000, /* QSPI supports higher frequencies */ \
        .trans_queue_depth = 10, \
        .on_color_trans_done = callback, \
        .user_ctx = callback_ctx, \
        .lcd_cmd_bits = 8, \
        .lcd_param_bits = 8, \
        .flags = { \
            .quad_mode = 1, /* Enable 4-line mode */ \
            .psram_dma_direct = 1, /* Optimization for PSRAM */ \
        }, \
    }

#ifdef __cplusplus
}
#endif
