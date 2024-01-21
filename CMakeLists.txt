cmake_minimum_required(VERSION 3.0)

project(entity_component_system)

set(COMPILED_FILES src/types.h src/dense_map.h src/red_black_tree.h src/main.cpp)
add_executable(${PROJECT_NAME} ${COMPILED_FILES})


