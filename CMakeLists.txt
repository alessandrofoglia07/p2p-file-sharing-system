cmake_minimum_required(VERSION 3.22)
project(p2p_file_sharing_system C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

include_directories(include)

set(RESOURCES src/main.c src/files.c src/socket.c
        src/handle_requests.c
        src/peers.c
        include/peers.h)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)

add_executable(p2p_file_sharing_system ${RESOURCES})

target_link_libraries(p2p_file_sharing_system pthread)