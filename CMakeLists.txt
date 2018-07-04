cmake_minimum_required (VERSION 2.6)

project(mcpelauncher-client LANGUAGES CXX)

add_executable(mcpelauncher-client src/main.cpp src/client_app_platform.cpp src/client_app_platform.h src/store.cpp src/store.h src/xbox_live_patches.cpp src/xbox_live_patches.h src/fake_jni.cpp src/fake_jni.h src/window_callbacks.cpp src/window_callbacks.h src/http_request_stub.cpp src/http_request_stub.h src/xbox_live_helper.cpp src/xbox_live_helper.h src/minecraft_gamepad_mapping.h src/splitscreen_patch.cpp src/splitscreen_patch.h src/cll_upload_auth_step.cpp src/cll_upload_auth_step.h src/gl_core_patch.cpp src/gl_core_patch.h src/xbox_sleep_shutdown_patch.cpp src/xbox_sleep_shutdown_patch.h)
target_link_libraries(mcpelauncher-client logger mcpelauncher-core gamewindow filepicker msa-daemon-client cll-telemetry argparser)

install(TARGETS mcpelauncher-client RUNTIME DESTINATION bin)