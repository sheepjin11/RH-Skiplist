cmake_minimum_required(VERSION 3.12)
project(skiplist_pmem)
add_executable(skiplist_pmem skiplist_pmem.cpp)

find_library(LIBHASHMAP
		NAMES libhashmap_rp.so
		HINTS /home/shrimp1/computer_archi/pmdk-1.7/src/examples/libpmemobj/hashmap 
		REQUIRED)
target_link_libraries(skiplist_pmem ${LIBHASHMAP})
target_link_libraries(skiplist_pmem pmem)
target_link_libraries(skiplist_pmem pmemobj)
target_link_libraries(skiplist_pmem pthread)
target_link_libraries(skiplist_pmem m)


