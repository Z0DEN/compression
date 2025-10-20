cmake_minimum_required(VERSION 3.16)
project(Huffman LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_library(huffmanlib STATIC src/lib/huffmanlib.cpp)

add_executable(Huffman src/main.cpp)

target_link_libraries(Huffman PRIVATE huffmanlib)
