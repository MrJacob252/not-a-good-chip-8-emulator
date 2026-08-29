cmake_minimum_required(VERSION 3.16)
project(chip8)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# Tell CMake where to look for custom package installations in /usr/local
list(APPEND CMAKE_PREFIX_PATH "/usr/local")

find_package(SDL3 REQUIRED)

# Create your game executable target as usual
add_executable(chip8 main.c)

# Link to the actual SDL3 library.
target_link_libraries(chip8 PRIVATE SDL3::SDL3)
