cmake_minimum_required(VERSION 3.17)
project(olcGameJam2021)

set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Finder)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_image REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Frameworks/SDL2.framework)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Frameworks/SDL2_ttf.framework)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Frameworks/SDL2_image.framework)

target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/Frameworks/SDL2.framework)
target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/Frameworks/SDL2_ttf.framework)
target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/Frameworks/SDL2_image.framework)