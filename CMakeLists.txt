cmake_minimum_required(VERSION 3.20)
project(CTextSearch LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(ctextsearch STATIC 
    src/text_search.c 
    src/builder_hashmap.c
    src/flatmap.c 
    src/array_list.c
)


target_sources(ctextsearch PRIVATE
    src/text_search.c
    src/array_list.c 
    src/builder_hashmap.c
    src/flatmap.c
)

target_include_directories(ctextsearch
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:inc>
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(text_search_cli cli/main.c)
target_link_libraries(text_search_cli PRIVATE ctextsearch)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

