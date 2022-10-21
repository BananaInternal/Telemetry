cmake_minimum_required(VERSION 3.21)

#############################################################################################
# Target for recompiling libraries
#############################################################################################
add_custom_target(BAN_additional_libs)
target_sources(BAN_additional_libs PRIVATE
    mac_install_name_tool.sh.in
    mac_setenv.sh.in
)

#############################################################################################
# options available in this subproject
#############################################################################################
option(BAN_USE_KUSERFEEDBACK "build libAppStore, fork of QtPurchasing for Banana+" ON)

#############################################################################################
# General settings for libraries
#############################################################################################
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    message(FATAL_ERROR "variable CMAKE_LIBRARY_OUTPUT_DIRECTORY not set")
endif()
if(BAN_PRECOMPILED_LIBS AND NOT EXISTS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    message(FATAL_ERROR "Pre compiled libs not found under ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
endif()
if(NOT EXISTS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    file(MAKE_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib)
    file(MAKE_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin)
    file(MAKE_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/include)
endif()

set(CMAKE_LIBRARY_TMP_SOURCES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/sources)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/include
)
target_link_directories(${CMAKE_PROJECT_NAME} PUBLIC 
    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib
)

include(ExternalProject)

if(UNIX OR APPLE)
    find_program(cmd_make make)
    if (NOT cmd_make)
        message(FATAL_ERROR "make not found")
    endif ()
endif()

if(APPLE)
    #On APPLE wit Externalproject_add, you always have to call
    #configuration for the new M1 chip, this is done in the mac_setenv.sh script
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/mac_setenv.sh.in
        ${CMAKE_LIBRARY_TMP_SOURCES}/mac_setenv.sh
        @ONLY
        FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    )
    set(cmd_standard_conf
        /bin/sh ${CMAKE_LIBRARY_TMP_SOURCES}/mac_setenv.sh
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/mac_install_name_tool.sh.in
        ${CMAKE_LIBRARY_TMP_SOURCES}/mac_install_name_tool.sh
        @ONLY
        FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    )
else()
    set(cmd_standard_conf "")
endif()

macro(run_install_name_tool a_extProjectName a_rootFileName)
    if(APPLE)
        ExternalProject_Add_Step(${a_extProjectName} ${a_extProjectName}_name_tool
            COMMAND /bin/sh ${CMAKE_LIBRARY_TMP_SOURCES}/mac_install_name_tool.sh ${a_rootFileName}
            WORKING_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib
            DEPENDEES install
        )
    endif()
endmacro()

if(WIN32)
    #on MSVC 17.3.2 not working anymore.
    #if("${CMAKE_EXE_LINKER_FLAGS}" STREQUAL "/machine:x64")
        set(win_cpu_type x64)
    #elseif("${CMAKE_EXE_LINKER_FLAGS}" STREQUAL "/machine:ARM64")
    #    set(c_cpu_type arm64)
    #endif()

endif()


#############################################################################################
# ZLib
# Free, general-purpose, legally unencumbered lossless data-compression library 
# for use on virtually any computer hardware and operating system
# needed by libHaru, KArchive and KUserFeedback
# https://www.zlib.net/
#############################################################################################
if(NOT BAN_PRECOMPILED_LIBS)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/zlib
        DESTINATION ${CMAKE_LIBRARY_TMP_SOURCES}
        FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    )
    ExternalProject_Add(zlib_make
        SOURCE_DIR ${CMAKE_LIBRARY_TMP_SOURCES}/zlib
        BUILD_IN_SOURCE ON
        CONFIGURE_COMMAND
            ${cmd_standard_conf}
            ${CMAKE_COMMAND} .
                -GNinja
                -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                -DCMAKE_BUILD_TYPE=MinSizeRel
                -DCMAKE_INSTALL_PREFIX=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        BUILD_COMMAND ${CMAKE_COMMAND} --build .
        INSTALL_COMMAND ${CMAKE_COMMAND} --install .
    )
    run_install_name_tool(zlib_make libz)
endif(NOT BAN_PRECOMPILED_LIBS)
list(APPEND cList_libs_to_add ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin/zlib${CMAKE_SHARED_LIBRARY_SUFFIX})



#############################################################################################
# KUserFeedback
# https://api.kde.org/frameworks/kuserfeedback/html/index.html
# Framework for collecting feedback from application users via telemetry and targeted surveys.
#############################################################################################
if(BAN_USE_KUSERFEEDBACK)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC
        BA_ENABLE_KUSERFEEDBACK
    )

    if(NOT BAN_PRECOMPILED_LIBS)
        message(STATUS "building KUserFeedback")

        ExternalProject_Add(
            c_extra_modules
            DEPENDS zlib_make
            DOWNLOAD_DIR ${CMAKE_LIBRARY_TMP_SOURCES}/cmake-extra
            #DOWNLOAD_EXTRACT_TIMESTAMP true
            URL https://github.com/KDE/extra-cmake-modules/archive/refs/heads/master.zip
            LIST_SEPARATOR |
            CMAKE_ARGS
                -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                -DCMAKE_BUILD_TYPE=MinSizeRel
                -DCMAKE_INSTALL_PREFIX=${CMAKE_LIBRARY_TMP_SOURCES}/extra-cmake-modules
        )

        set(karchive_prefix ${CMAKE_PREFIX_PATH})
        list(APPEND karchive_prefix
            ${CMAKE_LIBRARY_TMP_SOURCES}/extra-cmake-modules
        )
        string(REPLACE ";" "|" karchive_prefix "${karchive_prefix}")

        set(karchive_args
            -GNinja
            -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
            -DCMAKE_BUILD_TYPE=MinSizeRel
            -DCMAKE_PREFIX_PATH=${karchive_prefix}
            -DCMAKE_INSTALL_PREFIX=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
            -DQT_MAJOR_VERSION=6
            -DCMAKE_WARN_DEPRECATED=OFF
        )
        #Do not generate console, exe, docs..
        #only the libraries for the client
        list(APPEND karchive_args
            -DENABLE_DOCS=OFF
            -DENABLE_CONSOLE=OFF
            -DENABLE_CLI=OFF
            -DENABLE_PHP=OFF
            -DENABLE_PHP_UNIT=OFF
            -DENABLE_SURVEY_TARGET_EXPRESSIONS=OFF
        )

        if(APPLE)
            string(REPLACE ";" "|" apple_arch "${CMAKE_OSX_ARCHITECTURES}")
            list(APPEND karchive_args
                -DCMAKE_OSX_ARCHITECTURES=${apple_arch}
            )
        endif()

        ExternalProject_Add(kuserfeedback_make
            DOWNLOAD_DIR ${CMAKE_LIBRARY_TMP_SOURCES}/kuserfeedback
            #DOWNLOAD_EXTRACT_TIMESTAMP true
            URL https://invent.kde.org/libraries/kuserfeedback/-/archive/master/kuserfeedback-master.zip
            DEPENDS c_extra_modules
            BUILD_IN_SOURCE ON
            LIST_SEPARATOR |
            CMAKE_ARGS ${karchive_args}
        )
        add_dependencies(${CMAKE_PROJECT_NAME} kuserfeedback_make)
    endif(NOT BAN_PRECOMPILED_LIBS)
    if(NOT APPLE AND NOT WIN32)
        target_link_directories(${CMAKE_PROJECT_NAME} PUBLIC
            ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib/x86_64-linux-gnu
        )
    endif()
    target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC KUserFeedbackCore)
    target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC KUserFeedbackWidgets)
    target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
        "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/include/KUserFeedback"
    )
    list(APPEND cList_libs_to_add ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin/KUserFeedbackCore${CMAKE_SHARED_LIBRARY_SUFFIX})
    list(APPEND cList_libs_to_add ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin/KUserFeedbackWidgets${CMAKE_SHARED_LIBRARY_SUFFIX})
endif(BAN_USE_KUSERFEEDBACK)
