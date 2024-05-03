cmake_minimum_required(VERSION 2.8)
project(proxy)

add_definitions(-O0 -g2 -ggdb  -Wall -Wno-builtin-macro-redefined -Wno-deprecated)

if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION} VERSION_LESS 2.8.12)
    add_definitions(-std=c++0x)
else()
    add_compile_options(-std=c++0x)
endif()

set(PROJECT_ROOT_PATH           "${CMAKE_SOURCE_DIR}")
set(LIBRARY_OUTPUT_DIRECTORY    "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIRECTORY})
set(RUNTIME_OUTPUT_DIRECTORY    "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY})
 
include_directories(
	"${PROJECT_ROOT_PATH}/deps"
	"${PROJECT_ROOT_PATH}"
)

#link_directories(
#	"${LIBRARY_OUTPUT_DIRECTORY}"
#)

add_subdirectory(deps)

aux_source_directory(paxos PAXOS_SRC)

add_executable(node server.cpp main.cpp ${PAXOS_SRC})

target_link_libraries(node deps)