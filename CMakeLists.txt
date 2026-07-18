cmake_minimum_required(VERSION 3.15)

project(
    labwork9
    VERSION 1.0.0
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_CLANG_TIDY "Run clang-tidy during builds when available" ON)

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
    endif()
endif()

add_library(scheduler INTERFACE)
add_library(task_scheduler::scheduler ALIAS scheduler)

target_include_directories(scheduler
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/task-scheduler>
)

target_compile_features(scheduler INTERFACE cxx_std_23)

include(CTest)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/task-scheduler-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/task-scheduler-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/task-scheduler
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/task-scheduler-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/task-scheduler-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/task-scheduler-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/task-scheduler
)

install(
    TARGETS scheduler
    EXPORT schedulerTargets
)

install(
    FILES
        ${PROJECT_SOURCE_DIR}/include/task_scheduler/scheduler.hpp
        ${PROJECT_SOURCE_DIR}/include/task_scheduler/any.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/task-scheduler/task_scheduler
)

install(
    EXPORT schedulerTargets
    FILE task-scheduler-targets.cmake
    NAMESPACE task_scheduler::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/task-scheduler
)
