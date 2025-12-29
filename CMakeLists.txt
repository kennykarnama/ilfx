cmake_minimum_required(VERSION 3.10)
project(ilfx VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Xerces
find_path(XERCES_INCLUDE_DIR
        NAMES xercesc/util/PlatformUtils.hpp
        PATHS /opt/homebrew/include
        /usr/local/include
        /usr/include
)

find_library(XERCES_LIBRARIES
        NAMES xerces-c
        PATHS /opt/homebrew/lib
        /usr/local/lib
        /usr/lib
)


if(NOT XERCES_INCLUDE_DIR OR NOT XERCES_LIBRARIES)
    message(FATAL_ERROR "Xerces-C not found")
endif()

# Find all XSD schema files
file(GLOB XSD_SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/xsd/*.xsd")

# Custom target to generate C++ bindings from XSD schemas
add_custom_target(generate_bindings
    COMMAND xsd cxx-tree --generate-serialization 
            --output-dir ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp
            ${XSD_SCHEMAS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating C++ bindings from XSD schemas"
    VERBATIM
)

# Add source files
set(INHERENT_SOURCES
    gen/cpp/InherentDataSource.cxx
    gen/cpp/Weight.cxx
)

set(KPMR_SOURCES
    gen/cpp/KPMRDataSource.cxx
    gen/cpp/Weight.cxx
)

# Create main library for Inherent datasource
add_library(ilfx_lib ${INHERENT_SOURCES})
target_include_directories(ilfx_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp
    ${XERCES_INCLUDE_DIR}
)
target_link_libraries(ilfx_lib PUBLIC
    ${XERCES_LIBRARIES}
)

# Create library for KPMR datasource
add_library(kpmr_lib ${KPMR_SOURCES})
target_include_directories(kpmr_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp
    ${XERCES_INCLUDE_DIR}
)
target_link_libraries(kpmr_lib PUBLIC
    ${XERCES_LIBRARIES}
)

# Find required packages
find_package(GTest REQUIRED)

add_subdirectory(thirdparty/abseil-cpp)

# Add test executable
add_executable(inherent_datasource_test
    src/inherent_datasource_test.cpp
)

add_executable(chaicli_test src/chaicli_test.cpp)
target_include_directories(chaicli_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    ${XERCES_INCLUDE_DIR}
)
target_link_libraries(chaicli_test
    ilfx_lib
    ${XERCES_LIBRARIES}
    GTest::gtest
    GTest::gtest_main
    pthread
)
add_test(NAME ChaiCLITests COMMAND chaicli_test)

# Add CLI executable
add_executable(ilfx_cli src/ilfx_cli.cpp)
target_include_directories(ilfx_cli PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    ${XERCES_INCLUDE_DIR}
)
target_link_libraries(ilfx_cli
    ilfx_lib
    ${XERCES_LIBRARIES}
    absl::flags
    absl::flags_parse
    absl::log
    absl::log_initialize
    absl::log_internal_message
    pthread
)

# Add ChaiScript CLI executable
add_executable(chaiscript_cli src/chaiscript_cli.cpp)
target_include_directories(chaiscript_cli PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)
target_link_libraries(chaiscript_cli
    absl::flags
    absl::flags_parse
    absl::log
    absl::log_initialize
    pthread
)

# Add KPMR CLI executable
add_executable(kpmr_cli src/kpmr_cli.cpp)
target_include_directories(kpmr_cli PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    ${XERCES_INCLUDE_DIR}
)
target_link_libraries(kpmr_cli
    kpmr_lib
    ${XERCES_LIBRARIES}
    absl::flags
    absl::flags_parse
    absl::log
    absl::log_initialize
    absl::log_internal_message
    pthread
)

target_include_directories(inherent_datasource_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)

target_link_libraries(inherent_datasource_test
    ilfx_lib
    ${XERCES_LIBRARIES}
    GTest::gtest
    GTest::gtest_main
    pthread
    absl::base
    absl::log
)

# Enable testing
enable_testing()
add_test(NAME InherentDataSourceTests COMMAND inherent_datasource_test)

# Optional: Add all test discovery
gtest_discover_tests(inherent_datasource_test)

# Compiler flags
if(MSVC)
    target_compile_options(inherent_datasource_test PRIVATE /W4)
else()
    target_compile_options(inherent_datasource_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Print build info
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Output Directory: ${CMAKE_BINARY_DIR}")
