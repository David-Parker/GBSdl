cmake_minimum_required(VERSION 3.9)

project(GBSdl VERSION 0.0.1 DESCRIPTION "Nintendo GameBoy Emulator as a desktop application using SDL.")

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

file(COPY dep/GBLib/rom DESTINATION ${CMAKE_BINARY_DIR})
file(COPY connections.json DESTINATION ${CMAKE_BINARY_DIR})

add_subdirectory(dep/GBLib build/GBLib)
add_subdirectory(dep/SDL2 build/SDL2)
add_subdirectory(dep/jsoncpp build/jsoncpp)

add_executable(GBSdl
    src/main.cpp
    src/SDLEventHandler.cpp
    src/SDLGraphicsHandler.cpp
    src/SDLSerialHandler.cpp
    src/SDLEventHandler.h
    src/SDLGraphicsHandler.h
    src/SDLSerialHandler.h
)

target_include_directories(GBSdl PRIVATE src)
target_include_directories(GBSdl PRIVATE dep/GBLib/include)
target_include_directories(GBSdl PRIVATE dep/GBLib/interface)
target_include_directories(GBSdl PRIVATE dep/SDL2/include)
target_include_directories(GBSdl PRIVATE dep/jsoncpp/include)

target_link_libraries(GBSdl GBLib)
target_link_libraries(GBSdl SDL2)
target_link_libraries(GBSdl jsoncpp_lib)

if(WIN32)
  target_link_libraries(GBSdl wsock32 ws2_32)
endif()