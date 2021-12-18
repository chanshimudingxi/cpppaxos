cmake_minimum_required(VERSION 2.8)
project(proxy)

add_definitions(-ggdb3 -Werror -Wno-builtin-macro-redefined -Wno-deprecated)
add_definitions(-DIM_RUN_MODE)

if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION} VERSION_LESS 2.8.12)
    add_definitions(-std=c++0x)
else()
    add_compile_options(-std=c++0x)
endif()

set(PROJECT_ROOT_PATH           "${CMAKE_SOURCE_DIR}")
set(LIBRARY_OUTPUT_DIRECTORY    "${CMAKE_SOURCE_DIR}/lib")
set(RUNTIME_OUTPUT_DIRECTORY    "${CMAKE_SOURCE_DIR}/bin")
 
include_directories(
	"${PROJECT_ROOT_PATH}/deps"
	"${PROJECT_ROOT_PATH}"
)

link_directories(
	"${LIBRARY_OUTPUT_DIRECTORY}/"
)

add_subdirectory(deps)

aux_source_directory(proto PROTO_SRC)

add_executable(node node.cpp main.cpp ${PROTO_SRC})

target_link_libraries(node deps)