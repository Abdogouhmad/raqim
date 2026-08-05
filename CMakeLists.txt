cmake_minimum_required(VERSION 3.16)
project(raqim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(raqim
    src/main.cpp
    src/calculatorwindow.h
    src/calculatorwindow.cpp
    src/thememanager.h
    src/thememanager.cpp
)

target_compile_definitions(raqim PRIVATE APP_VERSION="${PROJECT_VERSION}")
target_link_libraries(raqim PRIVATE Qt6::Widgets)

# --- Installation -----------------------------------------------------------
# GNUInstallDirs gives us the standard, distro-correct destinations
# (CMAKE_INSTALL_BINDIR -> bin, CMAKE_INSTALL_DATADIR -> share, etc.)
# instead of hardcoding "usr/bin" and friends ourselves.
include(GNUInstallDirs)

install(TARGETS raqim
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES packaging/raqim.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES packaging/raqim.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)

# --- Uninstall target --------------------------------------------------------
# CMake has no built-in "uninstall" — this configures a script from the
# install_manifest.txt that `cmake --install` generates, then exposes it
# as a normal build target: `ninja uninstall` / `make uninstall`.
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        @ONLY
    )

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        COMMENT "Uninstalling raqim"
    )
endif()
