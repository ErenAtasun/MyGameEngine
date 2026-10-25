cmake_minimum_required(VERSION 3.20)
project(MakeMyEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Tüm çalıştırılabilirler tek klasörde: build/bin/<Config>/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(box2d CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_path(SOL2_INCLUDE_DIR sol/sol.hpp REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_path(MINIAUDIO_INCLUDE_DIR miniaudio.h REQUIRED)

set(MME_ASSETS_DIR ${CMAKE_SOURCE_DIR}/assets)

function(mme_set_warnings target)
    if(MSVC)
        # /utf-8: kaynaklar UTF-8 (Türkçe yorumlar), /W4: yüksek uyarı seviyesi.
        # <...> ile gelen üçüncü parti header'ların uyarıları bastırılır.
        target_compile_options(${target} PRIVATE /W4 /utf-8 /permissive- /external:anglebrackets /external:W0)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# Motoru kullanan bir oyun/örnek: mme_add_game(PingPong src/main.cpp ...)
function(mme_add_game target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} PRIVATE MME::Engine)
    mme_set_warnings(${target})
    if(MSVC)
        set_property(TARGET ${target} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${target}>)
    endif()
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${MME_ASSETS_DIR} $<TARGET_FILE_DIR:${target}>/assets
        COMMENT "Assets kopyalaniyor: ${target}")
endfunction()

add_subdirectory(engine)
add_subdirectory(games/PingPong)
add_subdirectory(games/TilemapDemo)
add_subdirectory(player)
add_subdirectory(editor)

option(MME_BUILD_TESTS "Motor birim testlerini derle" ON)
if(MME_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
