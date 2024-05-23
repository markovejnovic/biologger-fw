cmake_minimum_required(VERSION 3.20.4)

if (CONFIG_LICOR_TRISONICA_MINI)
    include_directories(.)
    target_sources(app PRIVATE li_550.c)
endif()
