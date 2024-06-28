cmake_minimum_required(VERSION 3.20.4)

if (CONFIG_XIMPEDANCE_AMP)
    include_directories(.)
    target_sources(app PRIVATE ximpedance_amp.c)
endif()
