cmake_minimum_required(VERSION 3.20)
project(MakeMyEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(MakeMyEngine src/main.cpp)

find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(MakeMyEngine PRIVATE
    glad::glad
    glfw
    glm::glm
    OpenGL::GL
)



add_custom_command(TARGET MakeMyEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:MakeMyEngine>/assets)

# include dizini
target_include_directories(MakeMyEngine PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external
)

# kaynak dosyalar
target_sources(MakeMyEngine PRIVATE
    src/main.cpp
    src/Renderer2D.cpp
    src/OrthoCamera.cpp
    src/OrthoCameraController.cpp

)

