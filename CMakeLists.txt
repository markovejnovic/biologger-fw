cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(floggy)

# Add your .c source files in this list.
target_sources(app PRIVATE src/main.c
                           src/observer.c
                           src/trutime.c
                           src/storage.c
                           src/experiment.c
                           # TODO(markovejnovic) Following 3 are hacks. The
                           # cmake spec should be in the ximpedance cmakelists
                           # but I can't get it to link.
                           drivers/sensor/ximpedance_amp/ximpedance_amp.c
                           drivers/sensor/ximpedance_amp/v2i_ximpedance22x_lut.c
                           drivers/sensor/ximpedance_amp/v2i_ximpedance10x_lut.c)

target_include_directories(app PRIVATE drivers)

add_subdirectory(drivers)
