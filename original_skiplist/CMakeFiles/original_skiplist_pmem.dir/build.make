# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.12

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/shrimp1/computer_archi/original_skiplist

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/shrimp1/computer_archi/original_skiplist

# Include any dependencies generated for this target.
include CMakeFiles/original_skiplist_pmem.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/original_skiplist_pmem.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/original_skiplist_pmem.dir/flags.make

CMakeFiles/original_skiplist_pmem.dir/main.c.o: CMakeFiles/original_skiplist_pmem.dir/flags.make
CMakeFiles/original_skiplist_pmem.dir/main.c.o: main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/shrimp1/computer_archi/original_skiplist/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/original_skiplist_pmem.dir/main.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/original_skiplist_pmem.dir/main.c.o   -c /home/shrimp1/computer_archi/original_skiplist/main.c

CMakeFiles/original_skiplist_pmem.dir/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/original_skiplist_pmem.dir/main.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/shrimp1/computer_archi/original_skiplist/main.c > CMakeFiles/original_skiplist_pmem.dir/main.c.i

CMakeFiles/original_skiplist_pmem.dir/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/original_skiplist_pmem.dir/main.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/shrimp1/computer_archi/original_skiplist/main.c -o CMakeFiles/original_skiplist_pmem.dir/main.c.s

# Object files for target original_skiplist_pmem
original_skiplist_pmem_OBJECTS = \
"CMakeFiles/original_skiplist_pmem.dir/main.c.o"

# External object files for target original_skiplist_pmem
original_skiplist_pmem_EXTERNAL_OBJECTS =

original_skiplist_pmem: CMakeFiles/original_skiplist_pmem.dir/main.c.o
original_skiplist_pmem: CMakeFiles/original_skiplist_pmem.dir/build.make
original_skiplist_pmem: /home/shrimp1/computer_archi/pmdk-1.7/src/examples/libpmemobj/list_map/libskiplist_map.so
original_skiplist_pmem: CMakeFiles/original_skiplist_pmem.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/shrimp1/computer_archi/original_skiplist/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable original_skiplist_pmem"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/original_skiplist_pmem.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/original_skiplist_pmem.dir/build: original_skiplist_pmem

.PHONY : CMakeFiles/original_skiplist_pmem.dir/build

CMakeFiles/original_skiplist_pmem.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/original_skiplist_pmem.dir/cmake_clean.cmake
.PHONY : CMakeFiles/original_skiplist_pmem.dir/clean

CMakeFiles/original_skiplist_pmem.dir/depend:
	cd /home/shrimp1/computer_archi/original_skiplist && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/shrimp1/computer_archi/original_skiplist /home/shrimp1/computer_archi/original_skiplist /home/shrimp1/computer_archi/original_skiplist /home/shrimp1/computer_archi/original_skiplist /home/shrimp1/computer_archi/original_skiplist/CMakeFiles/original_skiplist_pmem.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/original_skiplist_pmem.dir/depend

