#include <MME/MME.h>
#include "PongLayer.h"

int main() {
    mme::ApplicationSpec spec;
    spec.window.title = "MakeMyEngine - Ping Pong";
    spec.window.width = 1280;
    spec.window.height = 720;
    spec.uiFont = "fonts/FiraSans-Regular.ttf"; // Türkçe karakterli arayüz

    mme::Application app(spec);
    app.PushLayer<PongLayer>();
    app.Run();
    return 0;
}
