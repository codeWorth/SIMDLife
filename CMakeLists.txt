cmake_minimum_required(VERSION 3.10)
project(SIMDLife)

add_executable(gameoflife "src/main.cpp" "src/simd_life.cpp" "src/basic_life.cpp" "src/glad.c")
target_include_directories(gameoflife PRIVATE "include/")
target_link_directories(gameoflife PRIVATE "lib/lib-mingw-w64/")
target_link_libraries(gameoflife glfw3)

target_compile_options(gameoflife PUBLIC -mavx2 -O3 -march=native)
install(TARGETS gameoflife DESTINATION bin)