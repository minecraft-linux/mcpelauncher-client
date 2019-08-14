cmake_minimum_required (VERSION 2.6)

project(mcpelauncher-client LANGUAGES CXX ASM)

git_commit_hash(${CMAKE_CURRENT_SOURCE_DIR} CLIENT_GIT_COMMIT_HASH)
configure_file(src/build_info.h.in ${CMAKE_CURRENT_BINARY_DIR}/build_info/build_info.h)

add_executable(mcpelauncher-client src/main.cpp src/client_app_platform.cpp src/client_app_platform.h src/store.cpp src/store.h src/xbox_live_patches.cpp src/xbox_live_patches.h src/fake_jni.cpp src/fake_jni.h src/window_callbacks.cpp src/window_callbacks.h src/http_request_stub.cpp src/http_request_stub.h src/xbox_live_helper.cpp src/xbox_live_helper.h src/minecraft_gamepad_mapping.h src/splitscreen_patch.cpp src/splitscreen_patch.h src/cll_upload_auth_step.cpp src/cll_upload_auth_step.h src/gl_core_patch.cpp src/gl_core_patch.h src/tts_patch.cpp src/tts_patch.h src/hbui_patch.cpp src/hbui_patch.h src/utf8_util.h src/shader_error_patch.cpp src/shader_error_patch.h src/xbox_live_game_interface.cpp src/xbox_live_game_interface.h src/legacy/xbox_live_game_interface_legacy_1_2_3.cpp src/legacy/xbox_live_game_interface_legacy_1_2_3.h src/legacy/xbox_live_game_interface_legacy_1_4.cpp src/legacy/xbox_live_game_interface_legacy_1_4.h src/legacy/xbox_live_game_interface_legacy_1_2.cpp src/legacy/xbox_live_game_interface_legacy_1_2.h src/legacy/legacy_patches.cpp src/legacy/legacy_patches.h src/minecraft_game_wrapper.cpp src/minecraft_game_wrapper.h src/legacy/minecraft_game_wrapper_legacy.h src/legacy/xbox_live_game_interface_legacy_0_15_2.cpp src/legacy/xbox_live_game_interface_legacy_0_15_2.h)
target_link_libraries(mcpelauncher-client logger mcpelauncher-core gamewindow filepicker msa-daemon-client cll-telemetry argparser)
target_include_directories(mcpelauncher-client PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/build_info/)

if (IS_ARMHF_BUILD)
    target_sources(mcpelauncher-client PRIVATE src/armhf_support.cpp src/armhf_support.h)
    target_compile_definitions(mcpelauncher-client PRIVATE USE_ARMHF_SUPPORT)
else()
    target_sources(mcpelauncher-client PRIVATE src/cpuid.cpp src/cpuid.h src/xbox_shutdown_patch.cpp src/xbox_shutdown_patch.h src/xbox_shutdown_patch.S src/texel_aa_patch.cpp src/texel_aa_patch.h)
endif()

install(TARGETS mcpelauncher-client RUNTIME COMPONENT mcpelauncher-client DESTINATION bin)
include(CPackSettings.cmake)