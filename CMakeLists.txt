cmake_minimum_required (VERSION 2.6)

project(minecraft-client LANGUAGES CXX)

add_executable(minecraft-client src/main.cpp src/client_app_platform.cpp src/client_app_platform.h src/store.cpp src/store.h src/xbox_live_patches.cpp src/xbox_live_patches.h src/fake_jni.cpp src/fake_jni.h src/window_callbacks.cpp src/window_callbacks.h src/http_request_stub.cpp src/http_request_stub.h src/xbox_live_helper.cpp src/xbox_live_helper.h src/minecraft_gamepad_mapping.h)
target_link_libraries(minecraft-client logger mcpelauncher-core gamewindow filepicker msa-daemon-client)