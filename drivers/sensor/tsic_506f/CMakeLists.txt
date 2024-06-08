cmake_minimum_required(VERSION 3.20.4)

if (CONFIG_TSIC_506F)
    include_directories(.)
    target_sources(app PRIVATE tsic_506f.c)
endif()
