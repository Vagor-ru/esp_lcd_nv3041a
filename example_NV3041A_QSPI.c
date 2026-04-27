#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_nv3041a.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"

static const char *TAG = "nv3041a_qspi_example";

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

// Color definitions (RGB565)
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_YELLOW      0xFFE0
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F
#define COLOR_ORANGE      0xFA20
#define COLOR_PURPLE      0x8010
#define COLOR_GRAY        0x8410
#define COLOR_LIGHT_BLUE  0xA5DF

// Simple font 8x8 (minimal built-in font)
static const uint8_t font_8x8[][8] = {
    // Space
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // !
    {0x00, 0x00, 0x00, 0x20, 0x20, 0x00, 0x20, 0x00},
    // "
    {0x00, 0x50, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00},
    // #
    {0x14, 0x14, 0x7C, 0x14, 0x7C, 0x14, 0x14, 0x00},
    // $
    {0x20, 0x7C, 0x50, 0x38, 0x14, 0x78, 0x04, 0x00},
    // %
    {0x44, 0x44, 0x54, 0x54, 0x54, 0x54, 0x54, 0x00},
    // &
    {0x30, 0x48, 0x50, 0x20, 0x54, 0x88, 0x74, 0x00},
    // '
    {0x00, 0x20, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00},
    // (
    {0x00, 0x10, 0x20, 0x40, 0x40, 0x20, 0x10, 0x00},
    // )
    {0x00, 0x40, 0x20, 0x10, 0x10, 0x20, 0x40, 0x00},
    // *
    {0x14, 0x14, 0x7C, 0x14, 0x00, 0x00, 0x00, 0x00},
    // +
    {0x00, 0x20, 0x20, 0x7C, 0x20, 0x20, 0x00, 0x00},
    // ,
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x40},
    // -
    {0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00},
    // .
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00},
    // /
    {0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00},
    // 0-9
    {0x7C, 0x82, 0x82, 0x82, 0x82, 0x82, 0x7C, 0x00},  // 0
    {0x00, 0x20, 0x60, 0x20, 0x20, 0x20, 0x70, 0x00},  // 1
    {0x78, 0x84, 0x04, 0x18, 0x20, 0x40, 0xFC, 0x00},  // 2
    {0x78, 0x84, 0x04, 0x38, 0x04, 0x84, 0x78, 0x00},  // 3
    {0x08, 0x18, 0x28, 0x48, 0xFC, 0x08, 0x08, 0x00},  // 4
    {0xFC, 0x80, 0xF8, 0x04, 0x04, 0x84, 0x78, 0x00},  // 5
    {0x38, 0x40, 0x80, 0xF8, 0x84, 0x84, 0x78, 0x00},  // 6
    {0xFC, 0x04, 0x08, 0x10, 0x20, 0x40, 0x40, 0x00},  // 7
    {0x78, 0x84, 0x84, 0x78, 0x84, 0x84, 0x78, 0x00},  // 8
    {0x78, 0x84, 0x84, 0x7C, 0x04, 0x08, 0x70, 0x00},  // 9
    // :
    {0x00, 0x00, 0x20, 0x00, 0x00, 0x20, 0x00, 0x00},
};

// Draw pixel
static void draw_pixel(uint16_t *fb, int x, int y, uint16_t color, int width, int height) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        fb[y * width + x] = color;
    }
}

// Draw horizontal line
static void draw_h_line(uint16_t *fb, int x1, int x2, int y, uint16_t color, int width, int height) {
    if (y < 0 || y >= height) return;
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (x1 < 0) x1 = 0;
    if (x2 >= width) x2 = width - 1;
    for (int x = x1; x <= x2; x++) {
        fb[y * width + x] = color;
    }
}

// Draw vertical line
static void draw_v_line(uint16_t *fb, int x, int y1, int y2, uint16_t color, int width, int height) {
    if (x < 0 || x >= width) return;
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    if (y1 < 0) y1 = 0;
    if (y2 >= height) y2 = height - 1;
    for (int y = y1; y <= y2; y++) {
        fb[y * width + x] = color;
    }
}

// Draw line (Bresenham)
static void draw_line(uint16_t *fb, int x1, int y1, int x2, int y2, uint16_t color, int width, int height) {
    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx + dy;
    
    while (1) {
        draw_pixel(fb, x1, y1, color, width, height);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

// Draw rectangle
static void draw_rect(uint16_t *fb, int x1, int y1, int x2, int y2, uint16_t color, int width, int height) {
    draw_h_line(fb, x1, x2, y1, color, width, height);
    draw_h_line(fb, x1, x2, y2, color, width, height);
    draw_v_line(fb, x1, y1, y2, color, width, height);
    draw_v_line(fb, x2, y1, y2, color, width, height);
}

// Draw filled rectangle
static void draw_fill_rect(uint16_t *fb, int x1, int y1, int x2, int y2, uint16_t color, int width, int height) {
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    for (int y = y1; y <= y2; y++) {
        draw_h_line(fb, x1, x2, y, color, width, height);
    }
}

// Draw circle (midpoint)
static void draw_circle(uint16_t *fb, int cx, int cy, int r, uint16_t color, int width, int height) {
    int x = r;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        draw_pixel(fb, cx + x, cy + y, color, width, height);
        draw_pixel(fb, cx + y, cy + x, color, width, height);
        draw_pixel(fb, cx - y, cy + x, color, width, height);
        draw_pixel(fb, cx - x, cy + y, color, width, height);
        draw_pixel(fb, cx - x, cy - y, color, width, height);
        draw_pixel(fb, cx - y, cy - x, color, width, height);
        draw_pixel(fb, cx + y, cy - x, color, width, height);
        draw_pixel(fb, cx + x, cy - y, color, width, height);
        
        y += 1;
        if (err <= 0) {
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

// Draw filled circle
static void draw_fill_circle(uint16_t *fb, int cx, int cy, int r, uint16_t color, int width, int height) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x*x + y*y <= r*r) {
                draw_pixel(fb, cx + x, cy + y, color, width, height);
            }
        }
    }
}

// Draw character
static void draw_char(uint16_t *fb, int x, int y, char c, uint16_t fg_color, uint16_t bg_color, int width, int height) {
    int idx = (int)c;
    // Only support characters that are defined in font (space to 9)
    if (idx < 32 || idx > 57) idx = 0; // Space for undefined
    
    const uint8_t *char_data = &font_8x8[idx - 32][0];
    
    for (int row = 0; row < 8; row++) {
        uint8_t line = char_data[row];
        for (int col = 0; col < 8; col++) {
            if (line & (0x80 >> col)) {
                draw_pixel(fb, x + col, y + row, fg_color, width, height);
            } else if (bg_color != COLOR_BLACK) { // Transparent if black
                draw_pixel(fb, x + col, y + row, bg_color, width, height);
            }
        }
    }
}

// Draw string
static void draw_string(uint16_t *fb, int x, int y, const char *str, uint16_t fg_color, uint16_t bg_color, int width, int height) {
    int x_pos = x;
    while (*str) {
        draw_char(fb, x_pos, y, *str, fg_color, bg_color, width, height);
        x_pos += 8;
        str++;
    }
}

// Fill screen with color
static void fill_screen(uint16_t *fb, uint16_t color, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        fb[i] = color;
    }
}

// Demo function
static void run_graphics_demo(esp_lcd_panel_handle_t panel_handle) {
    // Allocate framebuffer
    size_t fb_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(uint16_t);
    uint16_t *fb = (uint16_t *)heap_caps_malloc(fb_size, MALLOC_CAP_DMA);
    if (!fb) {
        ESP_LOGE(TAG, "Failed to allocate framebuffer");
        return;
    }
    
    ESP_LOGI(TAG, "Running graphics demo...");
    
    // Demo 1: Clear screen and show text
    fill_screen(fb, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_string(fb, 10, 10, "NV3041A Demo", COLOR_WHITE, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_string(fb, 10, 30, "Graphics Test", COLOR_YELLOW, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, fb);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Demo 2: Lines
    fill_screen(fb, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_string(fb, 10, 10, "Lines Demo", COLOR_WHITE, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_line(fb, 10, 50, 200, 50, COLOR_RED, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_line(fb, 10, 80, 200, 150, COLOR_GREEN, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_line(fb, 10, 110, 200, 200, COLOR_BLUE, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_line(fb, 10, 140, 200, 250, COLOR_YELLOW, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_line(fb, 250, 50, 450, 250, COLOR_CYAN, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_line(fb, 450, 50, 250, 250, COLOR_MAGENTA, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, fb);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Demo 3: Rectangles
    fill_screen(fb, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_string(fb, 10, 10, "Rectangles Demo", COLOR_WHITE, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_rect(fb, 20, 50, 120, 150, COLOR_RED, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_rect(fb, 140, 50, 240, 150, COLOR_GREEN, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_rect(fb, 260, 50, 360, 150, COLOR_BLUE, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_rect(fb, 380, 50, 460, 150, COLOR_YELLOW, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_fill_rect(fb, 20, 170, 100, 220, COLOR_ORANGE, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_fill_rect(fb, 120, 170, 200, 220, COLOR_PURPLE, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_fill_rect(fb, 220, 170, 300, 220, COLOR_CYAN, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_fill_rect(fb, 320, 170, 400, 220, COLOR_LIGHT_BLUE, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, fb);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Demo 4: Circles
    fill_screen(fb, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_string(fb, 10, 10, "Circles Demo", COLOR_WHITE, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_circle(fb, 80, 120, 40, COLOR_RED, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_circle(fb, 200, 120, 50, COLOR_GREEN, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_circle(fb, 340, 120, 60, COLOR_BLUE, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_fill_circle(fb, 80, 200, 25, COLOR_YELLOW, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_fill_circle(fb, 200, 200, 35, COLOR_CYAN, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_fill_circle(fb, 340, 200, 45, COLOR_MAGENTA, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, fb);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Demo 5: Color bars
    fill_screen(fb, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_string(fb, 10, 10, "Color Bars", COLOR_WHITE, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    for (int i = 0; i < 8; i++) {
        uint16_t colors[] = {COLOR_WHITE, COLOR_YELLOW, COLOR_CYAN, COLOR_GREEN, COLOR_MAGENTA, COLOR_RED, COLOR_BLUE, COLOR_BLACK};
        draw_fill_rect(fb, i * 60, 40, (i + 1) * 60 - 1, 150, colors[i], EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    }
    // Gradient
    for (int x = 0; x < EXAMPLE_LCD_H_RES; x++) {
        uint8_t r = (x * 31) / EXAMPLE_LCD_H_RES;
        uint8_t g = (x * 63) / EXAMPLE_LCD_H_RES;
        uint8_t b = (x * 31) / EXAMPLE_LCD_H_RES;
        uint16_t color = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
        draw_v_line(fb, x, 160, 260, color, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    }
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, fb);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Demo 6: Moving text
    fill_screen(fb, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    for (int pos = EXAMPLE_LCD_H_RES; pos > -100; pos -= 2) {
        fill_screen(fb, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
        draw_string(fb, pos, 120, "Moving Text!", COLOR_GREEN, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
        esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, fb);
    }
    
    // Demo 7: Final message
    fill_screen(fb, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_string(fb, 10, 80, "Demo Complete!", COLOR_WHITE, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_string(fb, 10, 100, "NV3041A LCD", COLOR_YELLOW, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    draw_string(fb, 10, 120, "480x272 pixels", COLOR_CYAN, COLOR_BLACK, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, fb);
    
    free(fb);
    ESP_LOGI(TAG, "Graphics demo finished");
}

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
    
    // Run graphics demo
    run_graphics_demo(panel_handle);
    
    // Keep running
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}