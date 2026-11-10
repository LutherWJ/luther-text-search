cmake_minimum_required(VERSION 3.20)
project(C_TEXT_SEARCH LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(text_search)

target_sources(text_search PRIVATE
    src/text_search.c
    src/array_list.c 
    src/builder_hashmap.c
    src/main.c
)

target_include_directories(text_search PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

