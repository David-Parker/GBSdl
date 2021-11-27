cmake_minimum_required(VERSION 3.9)

project(SDL_net VERSION 2.0.1 DESCRIPTION "net library for Simple DirectMedia Layer.")

add_definitions(-DTCP_NODELAY) 
set(SDL2_DIR "../SDL2")
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1)

add_library(SDL_net SHARED
     SDL-net/SDL_net.h
     SDL-net/SDLnetsys.h
     SDL-net/SDLnet.c
     SDL-net/SDLnetselect.c
     SDL-net/SDLnetTCP.c
     SDL-net/SDLnetUDP.c
)

target_include_directories(SDL_net PRIVATE SDL-net)
target_include_directories(SDL_net PRIVATE ../SDL2/include)
target_link_libraries(SDL_net SDL2)

if(WIN32)
  target_link_libraries(SDL_net wsock32 ws2_32 iphlpapi)
endif()