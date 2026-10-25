# Görsel sahne editörü: projeyi açar, sahneleri düzenler, Lua script'li oyunu oynatır
mme_add_game(Editor
    src/main.cpp
    src/EditorLayer.cpp
    src/EditorPanels.cpp
    src/EditorViewport.cpp
)
# Proje seçicinin varsayılan klasörü (repodaki örnek projeler)
target_compile_definitions(Editor PRIVATE MME_PROJECTS_DIR="${CMAKE_SOURCE_DIR}/projects")
add_dependencies(Editor Player)  # "Oynatıcıda çalıştır" ve dışa aktarma Player.exe'yi kullanır
