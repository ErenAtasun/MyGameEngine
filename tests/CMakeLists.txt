add_executable(EngineTests EngineTests.cpp)
target_link_libraries(EngineTests PRIVATE MME::Engine)
# Motorun iç Input kancalarını (InputInternal.h) test edebilmek için
target_include_directories(EngineTests PRIVATE ${PROJECT_SOURCE_DIR}/engine/src)
target_compile_definitions(EngineTests PRIVATE MME_TEST_ASSETS_DIR="${MME_ASSETS_DIR}")
mme_set_warnings(EngineTests)

add_test(NAME EngineTests COMMAND EngineTests)

# Application döngüsü testleri: gizli pencere + OpenGL context ister (GPU gerekli)
add_executable(EngineAppTests AppTests.cpp)
target_link_libraries(EngineAppTests PRIVATE MME::Engine)
mme_set_warnings(EngineAppTests)
add_test(NAME EngineAppTests COMMAND EngineAppTests)
set_tests_properties(EngineAppTests PROPERTIES LABELS "gpu")
