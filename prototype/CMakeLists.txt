cmake_minimum_required(VERSION 3.16)

#############################################################################################
# Project base settings
# to be declared before other steps
#############################################################################################
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin_${CMAKE_BUILD_TYPE})

# Apple related
set(CMAKE_OSX_ARCHITECTURES arm64;x86_64)
set(MACOSX_DEPLOYMENT_TARGET 10.13.6)

project(mdi LANGUAGES CXX)

#############################################################################################
# Deployment base settings
# to be declared before other steps
#############################################################################################
#if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/deploy
        CACHE PATH "For this project, install creates the Installer package" FORCE
    )
#endif()
message(STATUS "Deployment will output to ${CMAKE_INSTALL_PREFIX}")

include(GNUInstallDirs)
if(APPLE)

    enable_language(OBJC OBJCXX)
    # crea un pacchetto .app
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_MACOSX_BUNDLE ON)

elseif(WIN32)
    set(CMAKE_WIN32_EXECUTABLE ON) ### applicazione win32 (senza console)

    #sotto windows ignorare GNUInstallDirs
    set(CMAKE_INSTALL_BINDIR .)
    set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_BINDIR})
    set(CMAKE_INSTALL_DATADIR ${CMAKE_INSTALL_BINDIR})

endif()


#############################################################################################
# Qt 6 COMPONENTS and SOURCE
#############################################################################################
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

#############################################################################################
# My binary
#############################################################################################
qt_add_executable(mdi
    main.cpp
    mainwindow.cpp mainwindow.h
    mdichild.cpp mdichild.h
    MANUAL_FINALIZATION
)

#############################################################################################
# LIBRARIES
#############################################################################################
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/ban_additional_libs)
add_subdirectory(libs)

target_link_libraries(mdi PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

#############################################################################################
# Resources
#############################################################################################
set(mdi_resource_files
    "images/copy.png"
    "images/cut.png"
    "images/new.png"
    "images/open.png"
    "images/paste.png"
    "images/save.png"
)

qt_add_resources(mdi "mdi"
    PREFIX
        "/"
    FILES
        ${mdi_resource_files}
)

qt_finalize_target(${CMAKE_PROJECT_NAME})

