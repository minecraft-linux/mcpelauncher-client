set(CPACK_PACKAGE_NAME "mcpelauncher-client")
set(CPACK_PACKAGE_VENDOR "mcpelauncher")
set(CPACK_PACKAGE_VERSION "${MANIFEST_GIT_COMMIT_HASH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A launcher for Minecraft: Pocket Edition")
set(CPACK_PACKAGE_CONTACT "https://github.com/minecraft-linux/mcpelauncher-manifest/issues")
set(CPACK_GENERATOR "TGZ;DEB")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)
set(CPACK_DEBIAN_PACKAGE_VERSION "${BUILD_TIMESTAMP}-${MANIFEST_GIT_COMMIT_HASH}")

set(CPACK_INSTALL_CMAKE_PROJECTS
        "${CMAKE_BINARY_DIR};mcpelauncher-bin-libs;mcpelauncher-bin-libs;/"
        "${CMAKE_BINARY_DIR};gamepad-mappings;gamepad-mappings;/"
        "${CMAKE_CURRENT_BINARY_DIR};mcpelauncher-client;mcpelauncher-client;/")
set(CPACK_OUTPUT_CONFIG_FILE CPackConfig.cmake)
if (DEB_OS_NAME MATCHES "ubuntu-xenial")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>=2.14), libstdc++6, libcurl3, libx11-6, zlib1g, libpng12-0, libevdev2, libudev1")
    set(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_DEBIAN_PACKAGE_VERSION}~xenial")
elseif(DEB_OS_NAME MATCHES "ubuntu-bionic")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>=2.14), libstdc++6, libcurl4, libx11-6, zlib1g, libpng16-16, libevdev2, libudev1")
elseif(DEB_OS_NAME MATCHES "ubuntu-disco")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>=2.14), libstdc++6, libcurl4, libx11-6, zlib1g, libpng16-16, libevdev2, libudev1")
    set(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_DEBIAN_PACKAGE_VERSION}~disco")
endif()

include(CPack)