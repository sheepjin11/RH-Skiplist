cmake_minimum_required(VERSION 3.12)
project(original_skiplist_pmem)
add_executable(original_skiplist_pmem main.c)

find_library(LIBHASHMAP
        NAMES libskiplist_map.so 
        HINTS /home/shrimp1/computer_archi/pmdk-1.7/src/examples/libpmemobj/list_map
        REQUIRED)
target_link_libraries(original_skiplist_pmem ${LIBHASHMAP})
target_link_libraries(original_skiplist_pmem pmem)
target_link_libraries(original_skiplist_pmem pmemobj)
target_link_libraries(original_skiplist_pmem pthread)
target_link_libraries(original_skiplist_pmem m)

