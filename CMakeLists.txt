cmake_minimum_required(VERSION 3.21)

project(cororing)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

option(ENABLE_TESTING OFF)
option(DISABLE_SANITIZERS OFF)
option(ENABLE_ASAN OFF)
option(ENABLE_UBSAN OFF)
option(ENABLE_TSAN OFF)
option(BUILD_DOCS "Build documentation" OFF)

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/submodules/andreasbuhr-cppcoro)

add_library(cororing STATIC src/ring.cpp src/sockets.cpp src/perftools.cpp)

target_include_directories(cororing PUBLIC ${PROJECT_INCLUDE_DIR})

target_link_libraries(cororing PUBLIC cppcoro uring)

if(NOT DISABLE_SANITIZERS)
    if(ENABLE_ASAN)
        add_compile_options(-fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()

    if(ENABLE_UBSAN)
        add_compile_options(-fsanitize=undefined)
        add_link_options(-fsanitize=undefined)
    endif()

    if(ENABLE_TSAN)
        add_compile_options(-fsanitize=thread)
        add_link_options(-fsanitize=thread)
    endif()
endif()

if (ENABLE_TESTING AND PROJECT_IS_TOP_LEVEL)
    enable_testing()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/submodules/unittestlib)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
endif()

if(BUILD_DOCS)
    find_package(Doxygen)

    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    add_custom_target(doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)

endif()
