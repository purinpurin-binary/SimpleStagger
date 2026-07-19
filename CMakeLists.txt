cmake_minimum_required(VERSION 3.21)
project(SimpleStagger VERSION 1.0.0 LANGUAGES CXX)

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(lib/commonlibsse-ng commonlib)

if(NOT COMMAND add_commonlibsse_plugin)
    include(lib/commonlibsse-ng/cmake/CommonLibSSE.cmake)
endif()

add_commonlibsse_plugin(${PROJECT_NAME}
    AUTHOR "SimpleStagger"
    SOURCES
        src/main.cpp
        src/Settings.cpp
        src/HitHandler.cpp
        src/Stagger.cpp
        src/Resistance.cpp
        src/MenuSMF.cpp
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor /permissive- /EHsc /MP /W4)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE src extern)

find_package(SimpleIni CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SimpleIni::SimpleIni)
