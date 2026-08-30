cmake_minimum_required(VERSION 3.21)
project(dcmmlib VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(DCMM_BUILD_SHARED "Build dcmmlib as a shared library" OFF)
option(DCMM_BUILD_TESTS "Build GoogleTest suite" ON)
option(DCMM_BUILD_CLI "Build dcmm-cli smoke tool" ON)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(WARNING "dcmmlib is developed for LLVM/Clang. Current compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

add_compile_options(-Wall -Wextra -Wno-unused-parameter)

if(WIN32)
  # getenv/fopen are the portable APIs; MSVC CRT marks them deprecated.
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

set(DCMM_SOURCES
  src/types.cpp
  src/path.cpp
  src/safety.cpp
  src/walk.cpp
  src/sha256.cpp
  src/catalog.cpp
  src/engine.cpp
  src/clean.cpp
  src/duplicates.cpp
  src/large_files.cpp
  src/space.cpp
  src/disk.cpp
  src/apps.cpp
  src/maintenance.cpp
  src/c_abi.cpp
)

if(DCMM_BUILD_SHARED)
  add_library(dcmm SHARED ${DCMM_SOURCES})
  target_compile_definitions(dcmm PRIVATE DCMM_EXPORTS)
  set_target_properties(dcmm PROPERTIES C_VISIBILITY_PRESET hidden CXX_VISIBILITY_PRESET hidden)
else()
  add_library(dcmm STATIC ${DCMM_SOURCES})
  target_compile_definitions(dcmm PUBLIC DCMM_STATIC)
endif()

add_library(dcmm::dcmm ALIAS dcmm)

target_include_directories(dcmm
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(APPLE)
  target_link_libraries(dcmm PUBLIC "-framework CoreFoundation")
elseif(WIN32)
  target_link_libraries(dcmm PUBLIC shell32)
endif()

set_target_properties(dcmm PROPERTIES
  OUTPUT_NAME dcmm
  VERSION ${PROJECT_VERSION}
  SOVERSION 1
)

if(DCMM_BUILD_CLI)
  add_executable(dcmm-cli cli/main.cpp)
  target_link_libraries(dcmm-cli PRIVATE dcmm)
endif()

if(DCMM_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  enable_testing()
  add_executable(dcmm-tests
    tests/test_format.cpp
    tests/test_path.cpp
    tests/test_safety.cpp
    tests/test_sha256.cpp
    tests/test_walk.cpp
    tests/test_catalog.cpp
    tests/test_duplicates.cpp
    tests/test_c_abi.cpp
    tests/test_disk.cpp
    tests/test_maintenance.cpp
  )
  target_link_libraries(dcmm-tests PRIVATE dcmm GTest::gtest_main)
  target_include_directories(dcmm-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  include(GoogleTest)
  gtest_discover_tests(dcmm-tests)
endif()

include(GNUInstallDirs)
install(TARGETS dcmm EXPORT dcmmTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/dcmm DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT dcmmTargets FILE dcmmTargets.cmake NAMESPACE dcmm::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dcmm)
