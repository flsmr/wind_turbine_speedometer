cmake_minimum_required(VERSION 3.7)

add_definitions(-std=c++17)

set(CXX_FLAGS "-pthread -Wall")

set(CMAKE_CXX_FLAGS, "${CXX_FLAGS}")

project(wind_turbine_speedometer)

find_package(Threads REQUIRED)

add_executable(wind_turbine_speedometer 
    src/main.cpp
    src/img_converter.cpp
    src/img_converter.h
    src/clustering.h
    src/clustering.cpp
    src/utility.h
    src/parallel_image_processor.h
    src/stb_image_write.h
    src/stb_image.h)

target_link_libraries(wind_turbine_speedometer Threads::Threads)
