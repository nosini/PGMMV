# These tests also build without the player's graphics/audio dependencies:
# cmake -S Player/proj.linux/tests -B build-tests
# cmake --build build-tests && ctest --test-dir build-tests --output-on-failure
cmake_minimum_required(VERSION 3.16)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(pgmmv_regression_tests CXX)
    enable_testing()
endif()
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(PLAYER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

add_executable(pgmmv_schema_tests LegacyProjectSchemaTest.cpp)
target_include_directories(pgmmv_schema_tests PRIVATE "${PLAYER_DIR}/Classes" "${PLAYER_DIR}/cocos2d/external")
target_compile_options(pgmmv_schema_tests PRIVATE -fpermissive)
add_test(NAME legacy_project_schema COMMAND pgmmv_schema_tests)

# Compile the production scissor methods against a small GL-state fixture. This
# exercises the actual transformations without a window or a graphics driver.
set(SCISSOR_SOURCE "${PLAYER_DIR}/cocos2d/external/ImGUI/IMGUIGLViewImpl.cpp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${SCISSOR_SOURCE}")
file(READ "${SCISSOR_SOURCE}" GLVIEW_SOURCE)
string(FIND "${GLVIEW_SOURCE}" "void IMGUIGLViewImpl::setScissorInPoints(" SCISSOR_BEGIN)
string(FIND "${GLVIEW_SOURCE}" "void IMGUIGLViewImpl::onGLFWError(" SCISSOR_END)
if(SCISSOR_BEGIN LESS 0 OR SCISSOR_END LESS SCISSOR_BEGIN)
    message(FATAL_ERROR "Cannot locate the production scissor methods")
endif()
math(EXPR SCISSOR_LENGTH "${SCISSOR_END} - ${SCISSOR_BEGIN}")
string(SUBSTRING "${GLVIEW_SOURCE}" ${SCISSOR_BEGIN} ${SCISSOR_LENGTH} SCISSOR_METHODS)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ScissorMethods.inc" "${SCISSOR_METHODS}")
add_executable(pgmmv_scissor_tests ScissorRectTest.cpp)
target_include_directories(pgmmv_scissor_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME scissor_rect COMMAND pgmmv_scissor_tests)

add_executable(pgmmv_audio_tests SimpleAudioEngineTest.cpp ../engine/SimpleAudioEngine-linux.cpp)
target_include_directories(pgmmv_audio_tests PRIVATE "${PLAYER_DIR}/cocos2d/cocos")
target_compile_definitions(pgmmv_audio_tests PRIVATE LINUX USE_AGTK)
# AudioEngine.h relies on these from the player's precompiled header.
target_precompile_headers(pgmmv_audio_tests PRIVATE <cstdint> <vector>)
add_test(NAME simple_audio_failed_effect COMMAND pgmmv_audio_tests failure)
add_test(NAME simple_audio_cleanup COMMAND pgmmv_audio_tests cleanup)
