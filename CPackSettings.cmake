set(CPACK_PACKAGE_NAME "mcpelauncher-client")
set(CPACK_PACKAGE_VENDOR "mcpelauncher")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A launcher for Minecraft: Pocket Edition")
set(CPACK_PACKAGE_CONTACT "https://github.com/minecraft-linux/mcpelauncher-manifest/issues")
set(CPACK_GENERATOR "TGZ;DEB")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)

set(CPACK_INSTALL_CMAKE_PROJECTS
        "${CMAKE_BINARY_DIR};mcpelauncher-bin-libs;mcpelauncher-bin-libs;/"
        "${CMAKE_CURRENT_BINARY_DIR};mcpelauncher-client;mcpelauncher-client;/")
set(CPACK_OUTPUT_CONFIG_FILE CPackConfig.cmake)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>=2.14), libcurl4")

include(CPack)