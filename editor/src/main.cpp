#include "EditorLayer.h"

// Kullanım: Editor.exe [proje klasörü]
// Klasör verilmezse proje seçici açılır (projects/ altındakiler listelenir).
int main(int argc, char** argv) {
    mme::ApplicationSpec spec;
    spec.window.title = "MakeMyEngine Editör";
    spec.window.width = 1600;
    spec.window.height = 900;
    spec.enableDebugTools = false;     // editörün kendi panelleri var
    spec.imguiDocking = true;
    spec.uiFont = "fonts/FiraSans-Regular.ttf";
    spec.clearColor = { 0.06f, 0.06f, 0.07f, 1.0f };

    mme::Application app(spec);
    app.PushLayer<EditorLayer>(argc > 1 ? argv[1] : "");
    app.Run();
}
