# ESP LCD NV3041A Driver

Драйвер LCD-панели NV3041A для ESP-IDF.

## Обзор

Этот компонент предоставляет драйвер для LCD-панели Novatek NV3041A с поддержкой интерфейсов SPI и QSPI. Панель NV3041A — это ЖК-дисплей с разрешением до 480x272 пикселей, часто используемый в проектах ESP32.

## Поддерживаемые функции

- **Интерфейсы**: SPI (3-line), QSPI (4-line)
- **Разрешение**: до 480x272
- **Цветовые форматы**: RGB565 (16 бит)
- **Управление**: сброс, инверсия цвета, поворот, отражение

## Требования

- ESP-IDF v4.4+
- ESP32/ESP32-S2/ESP32-S3/C3

## Установка

Добавьте этот компонент в ваш проект ESP-IDF через `idf.py add-dependency` или скопируйте в директорию компонентов.

## Использование

### 1. Подключение заголовочного файла

```c
#include "esp_lcd_nv3041a.h"
```

### 2. Инициализация SPI-шины

```c
// Пример для SPI
const spi_bus_config_t bus_config = 
    NV3041A_PANEL_BUS_SPI_CONFIG(
        .sclk_io_num = 8,
        .mosi_io_num = 9,
        .max_transfer_sz = 480 * 272 * 2
    );
ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO));
```

### 3. Создание панели IO

```c
esp_lcd_panel_io_handle_t io_handle = NULL;
const esp_lcd_panel_io_spi_config_t io_config = 
    NV3041A_PANEL_IO_SPI_CONFIG(
        .cs_gpio_num = 5,
        .dc_gpio_num = 6,
        // ... остальные параметры
    );
ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &io_handle));
```

### 4. Инициализация драйвера панели

```c
esp_lcd_panel_handle_t panel_handle = NULL;
const esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = 7,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
    .bits_per_pixel = 16,
};

ESP_ERROR_CHECK(esp_lcd_new_panel_nv3041a(io_handle, &panel_config, &panel_handle));
ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
```

## API Reference

### Основные функции

| Функция | Описание |
|---------|----------|
| `esp_lcd_new_panel_nv3041a()` | Создание новой панели NV3041A |
| `esp_lcd_panel_reset()` | Сброс панели |
| `esp_lcd_panel_init()` | Инициализация панели |
| `esp_lcd_panel_draw_bitmap()` | Вывод изображения |
| `esp_lcd_panel_disp_on_off()` | Включение/выключение дисплея |

### Макросы конфигурации

- `NV3041A_PANEL_BUS_SPI_CONFIG()` — конфигурация SPI-шины
- `NV3041A_PANEL_IO_SPI_CONFIG()` — конфигурация IO для SPI
- `NV3041A_PANEL_BUS_QSPI_CONFIG()` — конфигурация QSPI-шины
- `NV3041A_PANEL_IO_QSPI_CONFIG()` — конфигурация IO для QSPI

## Примеры

См. [example_NV3041A_QSPI.c](example_NV3041A_QSPI.c) для полного примера использования с QSPI.

## Версия

- **Версия**: 0.1.2
- **Совместимость**: ESP-IDF v4.4+

## Лицензия

Apache-2.0 License (см. [license.txt](license.txt))

## Ссылки

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/)
- [Novatek NV3041A Datasheet](https://www.novatek.com.tw)