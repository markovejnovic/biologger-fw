cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(floggy)

# Add your .c source files in this list.
target_include_directories(app PRIVATE 3rdparty)
target_sources(app PRIVATE src/main.c
                           src/observer.c
                           src/trutime.c
                           src/storage.c
                           src/experiment.c
                           src/python.cpp)
