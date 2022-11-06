cmake_minimum_required(VERSION 3.12)
project(Skiplist VERSION 1.1.1 LANGUAGES CXX)

# Executable will be in ../bin
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../bin)

#add_executable(skiplist skiplist.cc /home/heejin/pmdk-1.12.1/src/examples/libpmemobj/hashmap/hashmap_tx.c)
add_executable(skiplist skiplist.cc)
#link_directories(/home/heejin/pmdk-1.12.1/src/examples/libpmemobj/hashmap)
target_include_directories(skiplist PRIVATE /home/heejin/pmdk-1.12.1/src/examples/libpmemobj/hashmap)
target_link_libraries(skiplist /home/heejin/pmdk-1.12.1/src/examples/libpmemobj/hashmap/libhashmap_atomic.a)
target_link_libraries(skiplist pmem)
target_link_libraries(skiplist pmemobj)
target_link_libraries(skiplist pthread)
