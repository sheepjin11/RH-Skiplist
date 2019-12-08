cmake_minimum_required(VERSION 3.12)
project(Skiplist VERSION 1.1.1 LANGUAGES CXX)

# Executable will be in ../bin
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../bin)

add_executable(skiplist skiplist.cc)

find_package(Kuku 1.1.1 EXACT REQUIRED)
target_link_libraries(skiplist Kuku::kuku)

target_link_libraries(skiplist pmem)
target_link_libraries(skiplist pmemobj)
target_link_libraries(skiplist pthread)
