set(PROJECT_NAME EngineTemplate)

cmake_minimum_required(VERSION 3.20)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/engine/bolts.cpp
        src/engine/bolts.h
        src/glad.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
        glfw
        glm::glm
        "-framework OpenGL"
)
