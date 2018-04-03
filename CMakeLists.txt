cmake_minimum_required (VERSION 2.6)

project(minecraft-client LANGUAGES CXX)

add_executable(minecraft-client src/main.cpp)
target_link_libraries(minecraft-client logger mcpelauncher-core gamewindow)