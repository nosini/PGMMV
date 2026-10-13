# Linux build of the Pixel Game Maker MV player.
#
# The Windows build (proj.win32/player.sln) is the reference. Source lists are
# generated from its projects into proj.linux/sources.cmake by
# proj.linux/gen_sources.py; the settings below mirror its configurations.
#
#   cmake -S Player -B build -G Ninja -DPGMMV_CONFIGURATION=Runtime
#   cmake --build build

cmake_minimum_required(VERSION 3.16)

project(pgmmv-player C CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "This CMake build is for Linux only. Use proj.win32 on Windows.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only 64-bit builds are supported (the bundled SpiderMonkey is 64-bit).")
endif()

# Runtime: player shipped with built games ("Build Game" with debugging off).
# DebugMenuRuntime: shipped player with the debug menu.
# Release: editor test play. Debug: developer build.
set(PGMMV_CONFIGURATION "Runtime" CACHE STRING "Player configuration: Runtime, DebugMenuRuntime, Release or Debug")
set_property(CACHE PGMMV_CONFIGURATION PROPERTY STRINGS Runtime DebugMenuRuntime Release Debug)

if(NOT CMAKE_BUILD_TYPE)
    if(PGMMV_CONFIGURATION STREQUAL "Debug")
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
    else()
        set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
    endif()
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
# The prebuilt static libraries (SpiderMonkey, Bullet, ...) are not position independent.
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

include(proj.linux/sources.cmake)

set(ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cocos2d)
set(EXTERNAL_DIR ${ENGINE_DIR}/external)
set(PREBUILT_SUFFIX prebuilt/linux/64-bit)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS OpenGL)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED)
pkg_check_modules(SYSDEPS REQUIRED IMPORTED_TARGET
    glfw3 glew freetype2 fontconfig gtk+-3.0 libpng libjpeg libtiff-4 libwebp zlib
    libcurl sqlite3 openal vorbisfile vorbis ogg libmpg123 libvlc x11)

include(CTest)
if(BUILD_TESTING)
    add_executable(pgmmv_schema_tests proj.linux/tests/LegacyProjectSchemaTest.cpp)
    target_include_directories(pgmmv_schema_tests PRIVATE Classes ${EXTERNAL_DIR})
    # The bundled RapidJSON needs the same GCC compatibility flag as the player.
    target_compile_options(pgmmv_schema_tests PRIVATE -fpermissive)
    add_test(NAME legacy_project_schema COMMAND pgmmv_schema_tests)
endif()

# Prebuilt static libraries shipped with cocos2d-x.
function(pgmmv_prebuilt name path)
    add_library(${name} STATIC IMPORTED)
    set_target_properties(${name} PROPERTIES IMPORTED_LOCATION ${path})
endfunction()
pgmmv_prebuilt(pgmmv_mozjs ${EXTERNAL_DIR}/spidermonkey/${PREBUILT_SUFFIX}/libjs_static.a)
pgmmv_prebuilt(pgmmv_box2d ${EXTERNAL_DIR}/Box2D/${PREBUILT_SUFFIX}/libbox2d.a)
pgmmv_prebuilt(pgmmv_bullet_dynamics ${EXTERNAL_DIR}/bullet/${PREBUILT_SUFFIX}/libBulletDynamics.a)
pgmmv_prebuilt(pgmmv_bullet_collision ${EXTERNAL_DIR}/bullet/${PREBUILT_SUFFIX}/libBulletCollision.a)
pgmmv_prebuilt(pgmmv_bullet_linearmath ${EXTERNAL_DIR}/bullet/${PREBUILT_SUFFIX}/libLinearMath.a)
pgmmv_prebuilt(pgmmv_bullet_minicl ${EXTERNAL_DIR}/bullet/${PREBUILT_SUFFIX}/libMiniCL.a)
pgmmv_prebuilt(pgmmv_bullet_multithreaded ${EXTERNAL_DIR}/bullet/${PREBUILT_SUFFIX}/libBulletMultiThreaded.a)
pgmmv_prebuilt(pgmmv_websockets ${EXTERNAL_DIR}/websockets/${PREBUILT_SUFFIX}/libwebsockets.a)
pgmmv_prebuilt(pgmmv_uv ${EXTERNAL_DIR}/uv/${PREBUILT_SUFFIX}/libuv_a.a)

# Definitions shared by every part of the build. LINUX selects CC_PLATFORM_LINUX.
set(PGMMV_COMMON_DEFINITIONS
    LINUX
    USE_AGTK
    _USE_MATH_DEFINES
    GL_GLEXT_PROTOTYPES
    CC_ENABLE_CHIPMUNK_INTEGRATION=1
    LWS_WITH_LIBUV
    # From cocos2d_headers.props, imported by the engine, JS binding and player projects.
    IMAGE_MT_INIT=1
)
if(PGMMV_CONFIGURATION STREQUAL "Debug")
    list(APPEND PGMMV_COMMON_DEFINITIONS COCOS2D_DEBUG=1 _DEBUG)
else()
    list(APPEND PGMMV_COMMON_DEFINITIONS NDEBUG)
endif()

# Player definitions per configuration, as in proj.win32/player.vcxproj.
if(PGMMV_CONFIGURATION STREQUAL "Runtime")
    set(PGMMV_PLAYER_DEFINITIONS AGTK_RELEASE AGTK_PLAYER AGTK_RUNTIME)
elseif(PGMMV_CONFIGURATION STREQUAL "DebugMenuRuntime")
    set(PGMMV_PLAYER_DEFINITIONS AGTK_RELEASE AGTK_PLAYER AGTK_RUNTIME AGTK_PREVIEW)
elseif(PGMMV_CONFIGURATION STREQUAL "Release")
    set(PGMMV_PLAYER_DEFINITIONS AGTK_RELEASE AGTK_PLAYER)
elseif(PGMMV_CONFIGURATION STREQUAL "Debug")
    set(PGMMV_PLAYER_DEFINITIONS AGTK_DEBUG AGTK_PLAYER)
else()
    message(FATAL_ERROR "Unknown PGMMV_CONFIGURATION: ${PGMMV_CONFIGURATION}")
endif()

# Name and version the player reports (also to scripts), taken from the Windows
# version resource. Installer/build_player.bat renames "player" in it to the
# configuration name before building the runtimes.
file(STRINGS proj.win32/game.rc PGMMV_RC_VERSIONS REGEX "VALUE \"(File|Product)Version\"")
string(REGEX MATCH "\"FileVersion\", \"([0-9.]+)" _ "${PGMMV_RC_VERSIONS}")
set(PGMMV_RC_FILE_VERSION ${CMAKE_MATCH_1})
string(REGEX MATCH "\"ProductVersion\", \"([0-9.]+)" _ "${PGMMV_RC_VERSIONS}")
set(PGMMV_RC_PRODUCT_VERSION ${CMAKE_MATCH_1})
# AppDelegate uses the product version when the file version ends in ".0".
if(PGMMV_RC_FILE_VERSION MATCHES "\\.0$")
    set(PGMMV_APP_VERSION ${PGMMV_RC_PRODUCT_VERSION})
else()
    set(PGMMV_APP_VERSION ${PGMMV_RC_FILE_VERSION})
endif()
if(PGMMV_CONFIGURATION STREQUAL "Runtime")
    set(PGMMV_APP_NAME runtime)
elseif(PGMMV_CONFIGURATION STREQUAL "DebugMenuRuntime")
    set(PGMMV_APP_NAME debugMenuRuntime)
else()
    set(PGMMV_APP_NAME player)
endif()
message(STATUS "Player: ${PGMMV_APP_NAME} ${PGMMV_APP_VERSION}")

# The code was written for MSVC; keep GCC from rejecting what MSVC accepted.
set(PGMMV_COMPILE_OPTIONS
    -fno-strict-aliasing
    -fsigned-char
    $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>
    -Wno-deprecated-declarations
)

set(ENGINE_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/proj.linux/include
    ${ENGINE_DIR}
    ${ENGINE_DIR}/cocos
    ${ENGINE_DIR}/cocos/editor-support
    ${ENGINE_DIR}/cocos/platform
    ${ENGINE_DIR}/cocos/audio/include
    ${ENGINE_DIR}/extensions
    ${EXTERNAL_DIR}
    ${EXTERNAL_DIR}/unzip
    ${EXTERNAL_DIR}/edtaa3func
    ${EXTERNAL_DIR}/tinyxml2
    ${EXTERNAL_DIR}/chipmunk/include
    ${EXTERNAL_DIR}/xxhash
    ${EXTERNAL_DIR}/ConvertUTF
    ${EXTERNAL_DIR}/Box2D/include
    ${EXTERNAL_DIR}/poly2tri
    ${EXTERNAL_DIR}/poly2tri/common
    ${EXTERNAL_DIR}/poly2tri/sweep
    ${EXTERNAL_DIR}/bullet/include
    ${EXTERNAL_DIR}/bullet/include/bullet
    ${EXTERNAL_DIR}/websockets/include/linux
    ${EXTERNAL_DIR}/uv/include
    ${EXTERNAL_DIR}/spidermonkey/include/linux
    ${EXTERNAL_DIR}/ImGUI
)

# --- Chipmunk (built from the patched copy the Windows build uses) ---
add_library(pgmmv_chipmunk STATIC ${PGMMV_CHIPMUNK_SOURCES})
target_include_directories(pgmmv_chipmunk PRIVATE ${EXTERNAL_DIR}/chipmunk/include)
target_compile_definitions(pgmmv_chipmunk PRIVATE NDEBUG)
target_compile_options(pgmmv_chipmunk PRIVATE -fno-strict-aliasing)

# --- Recast/Detour ---
add_library(pgmmv_recast STATIC ${PGMMV_RECAST_SOURCES})
target_include_directories(pgmmv_recast PRIVATE ${EXTERNAL_DIR})
target_compile_definitions(pgmmv_recast PRIVATE USE_AGTK)

# --- cocos2d-x engine ---
add_library(pgmmv_cocos2d STATIC ${PGMMV_COCOS_SOURCES})
target_include_directories(pgmmv_cocos2d PUBLIC ${ENGINE_INCLUDE_DIRS})
target_compile_definitions(pgmmv_cocos2d PUBLIC ${PGMMV_COMMON_DEFINITIONS})
target_compile_options(pgmmv_cocos2d PUBLIC ${PGMMV_COMPILE_OPTIONS})
target_precompile_headers(pgmmv_cocos2d PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/proj.linux/precheader-linux.h>)
target_link_libraries(pgmmv_cocos2d PUBLIC
    pgmmv_recast
    pgmmv_chipmunk
    pgmmv_box2d
    pgmmv_bullet_dynamics
    pgmmv_bullet_collision
    pgmmv_bullet_multithreaded
    pgmmv_bullet_minicl
    pgmmv_bullet_linearmath
    pgmmv_websockets
    pgmmv_uv
    PkgConfig::SYSDEPS
    OpenGL::GL
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# --- JavaScript bindings ---
add_library(pgmmv_jscocos2d STATIC ${PGMMV_JSB_SOURCES})
target_include_directories(pgmmv_jscocos2d PUBLIC
    ${ENGINE_DIR}/cocos/scripting/js-bindings
    ${ENGINE_DIR}/cocos/scripting/js-bindings/auto
    ${ENGINE_DIR}/cocos/scripting/js-bindings/manual
    ${ENGINE_DIR}/cocos/scripting/js-bindings/manual/cocostudio
    ${ENGINE_DIR}/cocos/scripting/js-bindings/manual/navmesh
)
target_compile_definitions(pgmmv_jscocos2d PUBLIC COCOS2D_JAVASCRIPT=1)
target_precompile_headers(pgmmv_jscocos2d PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/proj.linux/precheader-linux.h>)
target_link_libraries(pgmmv_jscocos2d PUBLIC pgmmv_cocos2d pgmmv_mozjs)

# --- Player ---
add_executable(player ${PGMMV_PLAYER_SOURCES})
target_include_directories(player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/proj.linux
    ${CMAKE_CURRENT_SOURCE_DIR}/Classes
    ${CMAKE_CURRENT_SOURCE_DIR}/Classes/External
    ${CMAKE_CURRENT_SOURCE_DIR}/Classes/Manager
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${EXTERNAL_DIR}/chipmunk/include/chipmunk
)
target_compile_definitions(player PRIVATE ${PGMMV_PLAYER_DEFINITIONS} AGTKPLAYER_EXPORTS
    PGMMV_APP_NAME="${PGMMV_APP_NAME}" PGMMV_APP_VERSION="${PGMMV_APP_VERSION}")
# The Windows project force-includes stdafx.h into every C++ file.
target_precompile_headers(player PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/proj.linux/stdafx.h>)
target_link_libraries(player PRIVATE pgmmv_jscocos2d Boost::headers rt)
target_link_options(player PRIVATE -no-pie)

# The player's own resources, for game folders that don't include them (editor
# projects). The player looks in resources/ next to itself, then in
# ../share/pgmmv-player (see proj.linux/main.cpp).
set(PGMMV_PLAYER_RESOURCE_DIRS script plugins fonts img)
foreach(dir IN LISTS PGMMV_PLAYER_RESOURCE_DIRS)
    add_custom_command(TARGET player POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/Resources/${dir} $<TARGET_FILE_DIR:player>/resources/${dir})
endforeach()

include(GNUInstallDirs)
install(PROGRAMS $<TARGET_FILE:player> DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME pgmmv-player)
foreach(dir IN LISTS PGMMV_PLAYER_RESOURCE_DIRS)
    install(DIRECTORY Resources/${dir} DESTINATION ${CMAKE_INSTALL_DATADIR}/pgmmv-player)
endforeach()
install(FILES Resources/credits.html DESTINATION ${CMAKE_INSTALL_DATADIR}/pgmmv-player)
