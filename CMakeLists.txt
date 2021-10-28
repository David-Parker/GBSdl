cmake_minimum_required(VERSION 3.9)

project(GBSdl VERSION 0.0.1 DESCRIPTION "Nintendo GameBoy Emulator as a desktop application using SDL.")

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

file(COPY dep/GBLib/rom DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(dep/GBLib build/GBLib)
add_subdirectory(dep/SDL2 build/SDL2)

add_executable(GBSdl
    src/main.cpp
    src/SDLEventHandler.cpp
    src/SDLGraphicsHandler.cpp
    src/SDLEventHandler.h
    src/SDLGraphicsHandler.h
)

target_include_directories(GBSdl PRIVATE src)
target_include_directories(GBSdl PRIVATE dep/GBLib/include)
target_include_directories(GBSdl PRIVATE dep/SDL2/include)

target_link_libraries(GBSdl GBLib)
target_link_libraries(GBSdl SDL2)