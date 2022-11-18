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
option(BAN_USE_LIBHARU "build libHaru, PDF library" OFF)
option(BAN_USE_KARCHIVE "build KF5Archive, File compressing library" OFF)
option(BAN_USE_KUSERFEEDBACK "build libAppStore, fork of QtPurchasing for Banana+" OFF)
option(BAN_USE_LIBSODIUM "build libSodium, crypto library" OFF)
option(BAN_USE_LIBSTOOLDB "build StoolDB, synchronizable Json documents" OFF)
option(BAN_USE_QTWEBAPP "build QtWebApp, eb server by Stefan Frings" OFF)
option(BAN_USE_XERCES "build xerces-c, apache xml validator" OFF)
#option(BAN_USE_LIBAPPSTORE "build libAppStore, fork of QtPurchasing for Banana+" OFF)

if(BAN_USE_KARCHIVE OR BAN_USE_KUSERFEEDBACK)
    set(BAN_USING_KDE ON)
endif()
if(BAN_USE_LIBHARU OR BAN_USING_KDE)
    set(BAN_USING_ZLIB ON)
endif()

#############################################################################################
# General settings for libraries
#############################################################################################
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    message(FATAL_ERROR "variable CMAKE_LIBRARY_OUTPUT_DIRECTORY not set")
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

if(WIN32)
    #on MSVC 17.3.2 not working anymore.
    #if("${CMAKE_EXE_LINKER_FLAGS}" STREQUAL "/machine:x64")
        set(win_cpu_type x64)
    #elseif("${CMAKE_EXE_LINKER_FLAGS}" STREQUAL "/machine:ARM64")
    #    set(c_cpu_type arm64)
    #endif()

endif()

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
    set(cmd_setenv
        /bin/sh ${CMAKE_LIBRARY_TMP_SOURCES}/mac_setenv.sh
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/mac_install_name_tool.sh.in
        ${CMAKE_LIBRARY_TMP_SOURCES}/mac_install_name_tool.sh
        @ONLY
        FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    )
else()
    set(cmd_setenv "")
endif()


# Create a list with an alternate separator e.g. pipe symbol
string(REPLACE ";" "|" CMAKE_PREFIX_PATH_ALT_SEP "${CMAKE_PREFIX_PATH}")


#############################################################################################
# MACRO
# call install_name_tool after creation of an externalProject
# to change dynamic shared library install names after install
#############################################################################################
macro(run_install_name_tool a_extProjectName a_libName)
    if(APPLE)
        ExternalProject_Add_Step(${a_extProjectName} ${a_libName}_name_tool
            COMMAND /bin/sh ${CMAKE_LIBRARY_TMP_SOURCES}/mac_install_name_tool.sh ${a_libName}
            WORKING_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib
            DEPENDEES install
        )
    endif()
endmacro()



#############################################################################################
# ZLib
# Free, general-purpose, legally unencumbered lossless data-compression library
# for use on virtually any computer hardware and operating system
# needed by libHaru, KArchive and KUserFeedback
# https://www.zlib.net/
#############################################################################################
if(BAN_USING_ZLIB)

    message(STATUS "building ZLIB")

    ExternalProject_Add(zlib-download
      PREFIX downloads/zlib-download
      URL http://zlib.net/zlib1213.zip
      DOWNLOAD_NAME zlib1213.zip
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
    )
    ExternalProject_Get_Property(zlib-download download_dir download_name)
    ExternalProject_Add(
        zlib_make
        DEPENDS zlib-download
        URL ${download_dir}/${download_name}
        DOWNLOAD_DIR ${CMAKE_LIBRARY_TMP_SOURCES}/zlib
        BUILD_IN_SOURCE ON
        CONFIGURE_COMMAND
            ${cmd_setenv}
            ${CMAKE_COMMAND} .
                -GNinja
                -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                -DCMAKE_BUILD_TYPE=MinSizeRel
                -DCMAKE_INSTALL_PREFIX=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        BUILD_COMMAND ${CMAKE_COMMAND} --build .
        INSTALL_COMMAND ${CMAKE_COMMAND} --install .
    )
    run_install_name_tool(zlib_make libz)
    list(APPEND cList_libs_to_add ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin/zlib${CMAKE_SHARED_LIBRARY_SUFFIX})
    if(WIN32)
        set(c_zlibFileName ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib/zlib${CMAKE_STATIC_LIBRARY_SUFFIX})
    else()
        set(c_zlibFileName ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib/libz${CMAKE_STATIC_LIBRARY_SUFFIX})
    endif()
endif()



#############################################################################################
# LIBHARU
# Free, cross platform, open-sourced software library for generating PDF
# https://github.com/libharu
#############################################################################################
if(BAN_USE_LIBHARU)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC
        BA_ENABLE_LIBHARU_PDF_ENGINE
    )
    if(WIN32)
        set(c_haruLibName libhpdf)
    else()
        set(c_haruLibName hpdf)
    endif()

    message(STATUS "building LibHaru")

    set(c_libharuBinDir ${CMAKE_LIBRARY_TMP_SOURCES}/libharu)

    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/libpng
        DESTINATION ${c_libharuBinDir}
        FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    )
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/libharu-2.3.0/
        DESTINATION ${c_libharuBinDir}/libharu
        FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    )
    file(MAKE_DIRECTORY ${c_libharuBinDir}/libharu/lib)

    if(WIN32)
        find_program(cmd_nmake nmake)
        if (NOT cmd_nmake)
            MESSAGE(FATAL_ERROR "nmake not found")
        endif()

        set(c_haruMakeFile script/Makefile.msvc_dll.ban)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(c_haruMakeFile script/Makefile.msvc_dll_debug.ban)
            set(c_haruLibName libhpdfd)
        else()
        endif()

        set(c_pngConf
            ${CMAKE_COMMAND} -E remove_directory ${c_libharuBinDir}/zlib
            && ${CMAKE_COMMAND} -E copy_directory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/include ${c_libharuBinDir}/zlib
            && ${CMAKE_COMMAND} -E copy_directory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib/ ${c_libharuBinDir}/zlib/lib/
        )

        ExternalProject_Add(libpng
            DEPENDS             zlib_make
            SOURCE_DIR          ${c_libharuBinDir}/libpng
            BUILD_IN_SOURCE     ON
            CONFIGURE_COMMAND   ${c_pngConf}
            BUILD_COMMAND       ${cmd_nmake} -f scripts/makefile.vcwin32
            INSTALL_COMMAND     ${CMAKE_COMMAND} -E copy ${c_libharuBinDir}/libpng/libpng.lib ${c_libharuBinDir}/libpng/lib/libpng.lib
        )

        set(libharu_installCommand
            ${CMAKE_COMMAND} -E
                copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/libharu-2.3.0/include/ ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/include/
            && ${CMAKE_COMMAND} -E
                copy ${c_libharuBinDir}/libharu/lib/${c_haruLibName}${CMAKE_IMPORT_LIBRARY_SUFFIX} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib
            && ${CMAKE_COMMAND} -E
                copy ${c_libharuBinDir}/libharu/lib/${c_haruLibName}${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin
        )

        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            list(APPEND libharu_installCommand
                && ${CMAKE_COMMAND} -E
                    copy ${c_libharuBinDir}/libharu/lib/${c_haruLibName}.pdb ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib
            )
        endif()

        ExternalProject_Add(libharu_make
            DEPENDS libpng
            SOURCE_DIR          ${c_libharuBinDir}/libharu
            BUILD_IN_SOURCE     ON
            CONFIGURE_COMMAND   ${cmd_nmake} -f ${c_haruMakeFile} clean
            BUILD_COMMAND       ${cmd_nmake} -f ${c_haruMakeFile} PNG_PREFIX=../libpng ZLIB_PREFIX=../zlib
            INSTALL_COMMAND     ${libharu_installCommand}
        )
    else()
        if(APPLE)
            ExternalProject_Add(libpng_make
                DEPENDS zlib_make
                SOURCE_DIR ${c_libharuBinDir}/libpng
                BUILD_IN_SOURCE ON
                CONFIGURE_COMMAND
                    ${cmd_setenv}
                    ${c_libharuBinDir}/libpng/configure
                        --prefix=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                        --with-zlib-prefix=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                        --enable-arm-neon=no
                BUILD_COMMAND ${cmd_make}
                INSTALL_COMMAND ${cmd_make} install
            )

            ExternalProject_Add(libharu_make
                DEPENDS
                    zlib_make libpng_make
                SOURCE_DIR
                    ${c_libharuBinDir}/libharu
                BUILD_IN_SOURCE ON
                CONFIGURE_COMMAND
                    ${cmd_setenv}
                    ${CMAKE_COMMAND} ${c_libharuBinDir}/libharu
                        -GNinja
                        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                        -DCMAKE_BUILD_TYPE=MinSizeRel
                        -DCMAKE_PREFIX_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                        -DCMAKE_INSTALL_PREFIX=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                BUILD_COMMAND
                    ${CMAKE_COMMAND} --build ${c_libharuBinDir}/libharu
                INSTALL_COMMAND
                    ${CMAKE_COMMAND} --install ${c_libharuBinDir}/libharu
            )

        else()
            ExternalProject_Add(libharu_make
                SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libharu-2.3.0/
                BINARY_DIR ${CMAKE_LIBRARY_TMP_SOURCES}/libharu
                CMAKE_ARGS
                    -GNinja
                    -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                    -DCMAKE_BUILD_TYPE=MinSizeRel
                    -DCMAKE_INSTALL_PREFIX=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
            )
        endif()
    endif()
    run_install_name_tool(libharu_make libhpdf)
    add_dependencies(${CMAKE_PROJECT_NAME} libharu_make)

    target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${c_haruLibName})
    list(APPEND cList_libs_to_add ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin/${c_haruLibName}${CMAKE_SHARED_LIBRARY_SUFFIX})
else(BAN_USE_LIBHARU)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC
        BA_ENABLE_QT_PDF_ENGINE
    )
endif(BAN_USE_LIBHARU)



#############################################################################################
# LibSodium
# Modern, easy-to-use software library for encryption
# decryption, signatures, password hashing, and more
# https://doc.libsodium.org/
# https://download.libsodium.org/libsodium/releases/libsodium-1.0.18-stable.tar.gz
#############################################################################################
if(BAN_USE_LIBSODIUM)

    message(STATUS "building Sodium Crypto library")

    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC
        BA_USE_SODIUM
    )

    ExternalProject_Add(sodium-download
      PREFIX downloads/sodium-download
      URL https://download.libsodium.org/libsodium/releases/libsodium-1.0.18-stable.tar.gz
      DOWNLOAD_NAME sodium.tar.gz
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
    )
    ExternalProject_Get_Property(sodium-download download_dir download_name)
    set(libsodium_src_dir ${download_dir}/${download_name})
    set(libsodium_bin_dir ${CMAKE_LIBRARY_TMP_SOURCES}/sodium_bin)



    if (WIN32)
        set(sodium_cfg echo a) #cammands cannot be blank
        set(sodium_build
            msbuild builds/msvc/vs2022/libsodium.sln
            /m /v:n /p:Configuration=DynRelease /p:Platform=${win_cpu_type}
            /p:OutDir=${libsodium_bin_dir}/ /p:IntDir=${libsodium_bin_dir}/Inter/
        )
        set(sodium_install
            ${CMAKE_COMMAND} -E copy_directory ./src/libsodium/include/ ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/include/
            && ${CMAKE_COMMAND} -E copy ${libsodium_bin_dir}/libsodium${CMAKE_IMPORT_LIBRARY_SUFFIX} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib
            && ${CMAKE_COMMAND} -E copy ${libsodium_bin_dir}/libsodium${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin
        )
    elseif(APPLE)
        configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/libsodium/mac_sodium_configure.sh.in
            ${CMAKE_LIBRARY_TMP_SOURCES}/ban_configure.sh
            @ONLY
            FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
        )
        set(sodium_cfg
            ${CMAKE_COMMAND} -E copy ${CMAKE_LIBRARY_TMP_SOURCES}/ban_configure.sh . &&
            ./ban_configure.sh
        )
        set(sodium_build ${cmd_make})
        set(sodium_install ${cmd_make} install)
    else()
        #--enable-shared=no --enable-static=yes
        #--with-pthreads, enable thread support
        #--enable-minimal, minimal build without deprecated functions
        set(sodium_cfg
            ./configure
            --prefix=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
            --enable-minimal
            --with-pthreads
            --disable-debug
            --disable-dependency-tracking
        )
        set(sodium_build ${cmd_make})
        set(sodium_install ${cmd_make} install)
    endif()
    ExternalProject_Add(
        libsodium-make
        DEPENDS sodium-download
        URL ${libsodium_src_dir}
        BUILD_IN_SOURCE ON
        CONFIGURE_COMMAND ${sodium_cfg}
        BUILD_COMMAND ${sodium_build}
        INSTALL_COMMAND ${sodium_install}
    )

    add_dependencies(${CMAKE_PROJECT_NAME} libsodium-make)

    if(WIN32)
        target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC libsodium)
        list(APPEND cList_libs_to_add ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin/libsodium${CMAKE_SHARED_LIBRARY_SUFFIX})
    else()
        target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC sodium)
    endif()
endif(BAN_USE_LIBSODIUM)



#############################################################################################
# QtWebApp by Stefan Frings
# Is a HTTP server library in C++, inspired by Java Servlets.
# For Linux, Windows, Mac OS and many other operating systems that the Qt Framework supports.
# http://stefanfrings.de/qtwebapp/index-en.html
#############################################################################################
if(BAN_USE_QTWEBAPP)

    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC
        BA_USE_QTWEBAPP
    )
    message(STATUS "building QTWebapp")

    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/qtwebapp/QtWebApp
        DESTINATION ${CMAKE_LIBRARY_TMP_SOURCES}
    )

    ExternalProject_Add(qtwebapp_make
        SOURCE_DIR ${CMAKE_LIBRARY_TMP_SOURCES}/QtWebApp
        BUILD_IN_SOURCE ON
        LIST_SEPARATOR |
        CONFIGURE_COMMAND
            ${cmd_setenv}
            ${CMAKE_COMMAND}
                ${CMAKE_LIBRARY_TMP_SOURCES}/QtWebApp
                -GNinja
                -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                -DCMAKE_BUILD_TYPE=MinSizeRel
                -DCMAKE_INSTALL_PREFIX=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ALT_SEP}
        BUILD_COMMAND
            ${CMAKE_COMMAND} --build .
        INSTALL_COMMAND
            ${CMAKE_COMMAND} --install .
    )
    run_install_name_tool(qtwebapp_make libQtWebApp)
    add_dependencies(${CMAKE_PROJECT_NAME} qtwebapp_make)
    list(APPEND cList_libs_to_add ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin/QtWebApp${CMAKE_SHARED_LIBRARY_SUFFIX})
    target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC QtWebApp)

    #SSL certificates for webserver
    #andrebbero spostati sotto la cartella libs/qtwebapp
    if(APPLE)
        set(t_excludingPattern "win_*")
    elseif(WIN32)
        set(t_excludingPattern "mac_*")
    else()
        set(t_excludingPattern "*_certificates.*")
    endif()

    install(DIRECTORY ${CMAKE_SOURCE_DIR}/installer/Ssl
        DESTINATION ${CMAKE_INSTALL_DATADIR}
        PATTERN "*pfx" EXCLUDE
        PATTERN ${t_excludingPattern} EXCLUDE
    )
endif(BAN_USE_QTWEBAPP)



#############################################################################################
# StoolDB
# Enables the creation synchronizable Json documents.
# Library developed by SUPSI in collaboration with Banana.ch
# https://github.com/slashdotted/libstooldbpp
#############################################################################################
if(BAN_USE_LIBSTOOLDB)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC
        BA_ENABLE_BANANA_SYNC
    )

    message(STATUS "building libStoolDb")
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/libstooldbpp
        DESTINATION ${CMAKE_LIBRARY_TMP_SOURCES}
    )
    set(stooldb_BINARY_DIR ${CMAKE_LIBRARY_TMP_SOURCES}/libstooldbpp)

    if(WIN32)
        set(stooldb_installCommand
                ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/libstooldbpp/include/ ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/include/
                && ${CMAKE_COMMAND} -E copy ${stooldb_BINARY_DIR}/stooldbpp${CMAKE_IMPORT_LIBRARY_SUFFIX} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib
                && ${CMAKE_COMMAND} -E copy ${stooldb_BINARY_DIR}/stooldbpp${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin
        )
    else()
        set(stooldb_installCommand ${CMAKE_COMMAND} --install .)
    endif()
    ExternalProject_Add(stooldb_make
        SOURCE_DIR      ${stooldb_BINARY_DIR}
        BUILD_IN_SOURCE ON
        LIST_SEPARATOR  |
        CONFIGURE_COMMAND
            ${cmd_setenv}
            ${CMAKE_COMMAND}
                -GNinja
                -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                -DCMAKE_BUILD_TYPE=MinSizeRel
                -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ALT_SEP}
                -DCMAKE_INSTALL_PREFIX=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        BUILD_COMMAND   ${CMAKE_COMMAND} --build . --target=stooldbpp
        INSTALL_COMMAND ${stooldb_installCommand}
    )
    run_install_name_tool(stooldb_make libstool)
    add_dependencies(${CMAKE_PROJECT_NAME} stooldb_make)
    if(APPLE)
        #under apple we link it because of the oTool
        target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC stooldbpp)
    elseif(UNIX)
        #under unix we need it because of the appImage tool:
        #libraries are stripped
        target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC stooldbpp)
    endif()
    list(APPEND cList_libs_to_add ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin/stooldbpp${CMAKE_SHARED_LIBRARY_SUFFIX})
endif(BAN_USE_LIBSTOOLDB)



#############################################################################################
# KDE frameworks
# https://api.kde.org/
#############################################################################################
if(BAN_USING_KDE)

    set(c_kde_cmakeArgs
        -GNinja
        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
        -DCMAKE_BUILD_TYPE=MinSizeRel
    )
    if(NOT APPLE AND NOT WIN32)
        target_link_directories(${CMAKE_PROJECT_NAME} PUBLIC
            ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib/x86_64-linux-gnu
        )
    endif()

    #############################################################################################
    # KDE Extra CMake Modules (ECM)
    # https://api.kde.org/
    # Adds to the modules provided by CMake, including ones used by find_package()
    # to find common software, and common build settings used in software produced by the
    # KDE community.
    #############################################################################################
    ExternalProject_Add(
        c_extra_modules
        DEPENDS zlib_make
        DOWNLOAD_DIR ${CMAKE_LIBRARY_TMP_SOURCES}/cmake-extra
        #DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/KDE/extra-cmake-modules/archive/refs/heads/master.zip
        LIST_SEPARATOR |
        CMAKE_ARGS
            ${c_kde_cmakeArgs}
            -DCMAKE_INSTALL_PREFIX=${CMAKE_LIBRARY_TMP_SOURCES}/extra-cmake-modules
    )

    #add c_extra_modules to the prefix path
    set(t_kde_prefixPath ${CMAKE_PREFIX_PATH})
    list(APPEND t_kde_prefixPath
        ${CMAKE_LIBRARY_TMP_SOURCES}/extra-cmake-modules
    )
    string(REPLACE ";" "|" t_kde_prefixPath "${t_kde_prefixPath}")

    #set the common cmake args for KDE projects
    list(APPEND c_kde_cmakeArgs )
    list(APPEND c_kde_cmakeArgs
        -DQT_MAJOR_VERSION=${QT_DEFAULT_MAJOR_VERSION}
        -DCMAKE_PREFIX_PATH=${t_kde_prefixPath}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        -DZLIB_INCLUDE_DIR=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/include
    )
    if(APPLE)
        string(REPLACE ";" "|" t_arch "${CMAKE_OSX_ARCHITECTURES}")
        list(APPEND c_kde_cmakeArgs
            -DCMAKE_OSX_ARCHITECTURES=${t_arch}
        )
    endif()


    #############################################################################################
    # KAarchive
    # Reading, creating, and manipulating file archives
    # provides classes for easy reading, creation and manipulation of "archive" formats like ZIP and TAR.
    # It also provides transparent compression and decompression of data, like the GZip format, via a subclass of QIODevice.
    # this is a fork, made to compile with Qt6
    # https://api.kde.org/frameworks/karchive/html/index.html
    #############################################################################################
    if(BAN_USE_KARCHIVE)

        message(STATUS "building KF5Archive")
        ExternalProject_Add(KF5Archive-download
          PREFIX downloads/KF5Archive-download
          URL https://invent.kde.org/frameworks/karchive/-/archive/master/karchive-master.zip
          DOWNLOAD_NAME KF5Archive.zip
          CONFIGURE_COMMAND ""
          BUILD_COMMAND ""
          INSTALL_COMMAND ""
        )
        ExternalProject_Get_Property(KF5Archive-download download_dir download_name)
        ExternalProject_Add(KF5Archive_make
            DEPENDS c_extra_modules KF5Archive-download
            URL ${download_dir}/${download_name}
            BUILD_IN_SOURCE ON
            LIST_SEPARATOR |
            CONFIGURE_COMMAND
                ${cmd_setenv} echo "setting Environment" &&
                ${CMAKE_COMMAND} . ${c_kde_cmakeArgs}
            BUILD_COMMAND
                ${CMAKE_COMMAND} --build .
            INSTALL_COMMAND
                ${CMAKE_COMMAND} --install .
            )
        add_dependencies(${CMAKE_PROJECT_NAME} KF5Archive_make)

        target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC
            BA_ENABLE_KARCHIVE
        )
        run_install_name_tool(KF5Archive_make libKF5Archive)
        target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC KF5Archive)
        list(APPEND cList_libs_to_add ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin/KF5Archive${CMAKE_SHARED_LIBRARY_SUFFIX})
        target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
            "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/include/KF6/KArchive"
        )
    endif(BAN_USE_KARCHIVE)

    #############################################################################################
    # KUserFeedback
    # https://api.kde.org/frameworks/kuserfeedback/html/index.html
    # Framework for collecting feedback from application users via telemetry and targeted surveys.
    #############################################################################################
    if(BAN_USE_KUSERFEEDBACK)

        message(STATUS "building KF5Archive")

        set(c_kuf_args ${c_kde_cmakeArgs})
        list(APPEND c_kuf_args
            -DENABLE_DOCS=OFF
            -DENABLE_CONSOLE=OFF
            -DENABLE_CLI=OFF
            -DENABLE_PHP=OFF
            -DENABLE_PHP_UNIT=OFF
            -DENABLE_SURVEY_TARGET_EXPRESSIONS=OFF
        )

        ExternalProject_Add(kuserfeedback_make
            DOWNLOAD_DIR ${CMAKE_LIBRARY_TMP_SOURCES}/kuserfeedback
            #DOWNLOAD_EXTRACT_TIMESTAMP true
            URL https://invent.kde.org/libraries/kuserfeedback/-/archive/master/kuserfeedback-master.zip
            DEPENDS c_extra_modules
            BUILD_IN_SOURCE ON
            LIST_SEPARATOR |
            CMAKE_ARGS ${c_kuf_args}
        )

        add_dependencies(${CMAKE_PROJECT_NAME} kuserfeedback_make)
        target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC
            BA_ENABLE_KUSERFEEDBACK
        )
        target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC KUserFeedbackCore)
        target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC KUserFeedbackWidgets)
        target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
            "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/include/KUserFeedback"
        )
        run_install_name_tool(kuserfeedback_make libKUserFeedbackCore)
        run_install_name_tool(kuserfeedback_make libKUserFeedbackWidgets)
        list(APPEND cList_libs_to_add ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin/KUserFeedbackCore${CMAKE_SHARED_LIBRARY_SUFFIX})
        list(APPEND cList_libs_to_add ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin/KUserFeedbackWidgets${CMAKE_SHARED_LIBRARY_SUFFIX})
    endif(BAN_USE_KUSERFEEDBACK)

endif(BAN_USING_KDE)



#############################################################################################
# XERCES
# Validating XML parser written in a portable subset of C++
# https://xerces.apache.org/
#############################################################################################
if(BAN_USE_XERCES)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC
        BA_ENABLE_XERCES
    )
    message(STATUS "building XERCES")

    ExternalProject_Add(xerces-download
          PREFIX downloads/xerces-download
          URL https://dlcdn.apache.org//xerces/c/3/sources/xerces-c-3.2.4.zip
          DOWNLOAD_NAME xerces.zip
          CONFIGURE_COMMAND ""
          BUILD_COMMAND ""
          INSTALL_COMMAND ""
        )
    ExternalProject_Get_Property(xerces-download download_dir download_name)
    ExternalProject_Add(xerces_make
        DEPENDS xerces-download
        URL ${download_dir}/${download_name}
        BUILD_IN_SOURCE ON
        LIST_SEPARATOR |
        PATCH_COMMAND
            ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/xerces-c/CMakeLists-patched.txt ./CMakeLists.txt
        CONFIGURE_COMMAND
            ${cmd_setenv}
            ${CMAKE_COMMAND} .
                -GNinja
                -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                -DCMAKE_BUILD_TYPE=MinSizeRel
                -DCMAKE_INSTALL_PREFIX=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ALT_SEP}
                -DBUILD_SHARED_LIBS:BOOL=ON
                -Dnetwork:BOOL=OFF
        BUILD_COMMAND
            ${CMAKE_COMMAND} --build . --target xerces-c
        INSTALL_COMMAND
            ${CMAKE_COMMAND} --install .
    )
    run_install_name_tool(xerces_make libxerces)
    add_dependencies(${CMAKE_PROJECT_NAME} xerces_make)
    target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
        "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/include/xercesc"
    )
    if(WIN32)
        target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC xerces-c_3)
        list(APPEND cList_libs_to_add ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin/xerces-c_3_2${CMAKE_SHARED_LIBRARY_SUFFIX})
    else()
        target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC xerces-c)
    endif()

endif(BAN_USE_XERCES)


#############################################################################################
# OPEN SSL
#############################################################################################
if(WIN32)
    set(t_openSSL_lib_name libssl-1_1-${win_cpu_type}${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(t_openSSL_crypto_name libcrypto-1_1-${win_cpu_type}${CMAKE_SHARED_LIBRARY_SUFFIX})
    find_package(OpenSSL QUIET)
    if(OPENSSL_FOUND)
        #copy in the %path% (see .vs/launch.json)
        set(c_openSSLdllsDir ${OPENSSL_INCLUDE_DIR}/../bin)
    else()
        message(STATUS "OPENSSL -> Using project pre compiled libs")
        set(c_openSSLdllsDir ${CMAKE_CURRENT_SOURCE_DIR}/openssl/win/)
    endif()
    if(c_openSSLdllsDir AND CMAKE_INSTALL_LIBDIR)
        file(COPY_FILE
            ${c_openSSLdllsDir}/${t_openSSL_lib_name}
            ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin/${t_openSSL_lib_name}
            ONLY_IF_DIFFERENT
        )
        file(COPY_FILE
            ${c_openSSLdllsDir}/${t_openSSL_crypto_name}
            ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin/${t_openSSL_crypto_name}
            ONLY_IF_DIFFERENT
        )
    endif()
elseif(ANDROID)
    message(WARNING "Android needs OpenSSL to be distributed")
elseif(APPLE)

elseif(UNIX)
    #Ubuntu22 uses OpenSSL 3... but Qt still uses OpenSSL 1.1.1
    #we need to distribute SSL
    message(WARNING "Ubuntu22 needs OpenSSL 1.1.1 to be distributed")
endif()

if(t_openSSL_lib_name AND CMAKE_INSTALL_LIBDIR)
    install(FILES
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin/${t_openSSL_lib_name}
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bin/${t_openSSL_crypto_name}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

#############################################################################################
# System dependent libraries
#############################################################################################
if(APPLE)
    target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC
        "-framework AppKit"
        "-framework Carbon"
        "-framework CoreFoundation"
        "-framework CoreServices"
    )
endif()

#############################################################################################
# Installing all the above libraries
#############################################################################################
if(WIN32 AND cList_libs_to_add)
    install(PROGRAMS
            ${cList_libs_to_add}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT System
    )
endif()
