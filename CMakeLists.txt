cmake_minimum_required (VERSION 2.6)

project(minecraft-client LANGUAGES CXX)

add_executable(minecraft-client src/main.cpp src/client_app_platform.cpp src/client_app_platform.h src/store.cpp src/store.h)
target_link_libraries(minecraft-client logger mcpelauncher-core gamewindow filepicker)