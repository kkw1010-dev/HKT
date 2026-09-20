cmake_minimum_required(VERSION 3.25)

project(CIGAR VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# CommonLibSSE-NG (alandtse/CommonLibVR, branch ng) from the submodule, built from source.
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/commonlibsse-ng commonlib EXCLUDE_FROM_ALL)
if(NOT COMMAND add_commonlibsse_plugin)
	include(lib/commonlibsse-ng/cmake/CommonLibSSE.cmake)
endif()

# A release build makes the control panel player-facing: it drops the internal module names,
# the live gate strings and the last log line, and shows what each module does instead.
option(CIGAR_RELEASE "Player-facing control panel (no author-side diagnostics)" OFF)

find_package(nlohmann_json CONFIG REQUIRED)

file(GLOB_RECURSE CIGAR_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_commonlibsse_plugin(${PROJECT_NAME}
	AUTHOR "TAKEALOOK"
	SOURCES ${CIGAR_SOURCES}
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor /utf-8 /permissive- /EHsc /W4)
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE
	$<$<BOOL:${CIGAR_RELEASE}>:CIGAR_RELEASE>)
target_include_directories(${PROJECT_NAME} PRIVATE src include)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
