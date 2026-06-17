cmake_minimum_required(VERSION 3.20)
project(gb_emulator CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)

add_library(gb_core
    src/cpu.cpp src/mmu.cpp src/ppu.cpp
    src/timer.cpp src/cartridge.cpp src/joypad.cpp src/gameboy.cpp
    src/rom_only_cartridge.cpp src/cartridge_factory.cpp src/interrupt_controller.cpp
)

target_include_directories(gb_core PUBLIC src)
# will likely need something like this we begin graphics
# target_link_libraries(gb_core PUBLIC SDL2::SDl2 SDL2::SDL2main)

enable_testing()
add_subdirectory(tests)

add_executable(gb
  src/main.cpp
)

target_include_directories(gb PRIVATE src)
target_link_libraries(gb PRIVATE SDL2::SDL2 SDL2::SDL2main)
target_link_libraries(gb PRIVATE gb_core)

target_compile_options(gb PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>)
