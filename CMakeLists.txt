cmake_minimum_required(VERSION 3.16)

idf_component_register(
    SRCS "esp_lcd_nv3041a.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_lcd
    PRIV_REQUIRES driver
)

include(package_manager)
cu_pkg_define_version(${CMAKE_CURRENT_LIST_DIR})
