cmake_minimum_required (VERSION 2.6)

project(mcpelauncher-client LANGUAGES CXX ASM)

git_commit_hash(${CMAKE_CURRENT_SOURCE_DIR} CLIENT_GIT_COMMIT_HASH)
configure_file(src/build_info.h.in ${CMAKE_CURRENT_BINARY_DIR}/build_info/build_info.h)

add_executable(mcpelauncher-client src/main.cpp src/main.h src/window_callbacks.cpp src/window_callbacks.h src/xbox_live_helper.cpp src/xbox_live_helper.h src/splitscreen_patch.cpp src/splitscreen_patch.h src/cll_upload_auth_step.cpp src/cll_upload_auth_step.h src/gl_core_patch.cpp src/gl_core_patch.h src/hbui_patch.cpp src/hbui_patch.h src/utf8_util.h src/shader_error_patch.cpp src/shader_error_patch.h src/jni/jni_descriptors.cpp src/jni/java_types.h src/jni/main_activity.cpp src/jni/main_activity.h src/jni/store.h src/jni/jni_support.h src/jni/jni_support.cpp src/fake_looper.cpp src/fake_looper.h src/fake_assetmanager.cpp src/fake_assetmanager.h src/fake_egl.cpp src/fake_egl.h src/fake_inputqueue.cpp src/fake_inputqueue.h src/symbols.cpp src/symbols.h src/text_input_handler.cpp src/text_input_handler.h src/jni/xbox_live.cpp src/jni/xbox_live.h src/core_patches.cpp src/core_patches.h)
target_link_libraries(mcpelauncher-client logger mcpelauncher-core gamewindow filepicker msa-daemon-client cll-telemetry argparser baron android-support-headers)
target_include_directories(mcpelauncher-client PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/build_info/)

if (IS_ARMHF_BUILD)
    target_sources(mcpelauncher-client PRIVATE src/armhf_support.cpp src/armhf_support.h)
    target_compile_definitions(mcpelauncher-client PRIVATE USE_ARMHF_SUPPORT)
else()
    target_sources(mcpelauncher-client PRIVATE src/cpuid.cpp src/cpuid.h src/xbox_shutdown_patch.cpp src/xbox_shutdown_patch.h src/xbox_shutdown_patch.S src/texel_aa_patch.cpp src/texel_aa_patch.h)
endif()

install(TARGETS mcpelauncher-client RUNTIME COMPONENT mcpelauncher-client DESTINATION bin)
include(CPackSettings.cmake)