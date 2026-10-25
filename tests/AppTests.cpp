// Application döngüsünün testleri: gerçek (gizli) bir pencere ve OpenGL context ister.
// GPU'suz ortamlarda yalnızca EngineTests çalıştırılabilir.
#include <MME/MME.h>

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace {
    int g_Failures = 0;

    void Check(bool ok, const char* expr, int line) {
        if (ok) return;
        ++g_Failures;
        std::printf("    FAIL AppTests.cpp:%d  %s\n", line, expr);
    }
#define CHECK(cond) Check((cond), #cond, __LINE__)

    mme::ApplicationSpec HiddenSpec() {
        mme::ApplicationSpec spec;
        spec.window.title = "MME AppTests";
        spec.window.width = 320;
        spec.window.height = 240;
        spec.window.visible = false;
        spec.window.vsync = false;
        spec.enableDebugTools = false;
        spec.enableAudio = false;
        spec.hotReload = false;
        return spec;
    }

    std::vector<std::string> g_Log;

    struct Probe : mme::Layer {
        explicit Probe(std::string n) : Layer(n) {}
        void OnAttach() override { g_Log.push_back("attach " + GetName()); }
        void OnDetach() override { g_Log.push_back("detach " + GetName()); }
        void OnUpdate(float) override { g_Log.push_back("update " + GetName()); }
        int fixedSteps = 0;
        void OnFixedUpdate(float) override { ++fixedSteps; }
    };

    // Her frame bir adım çalıştıran test sürücüsü
    struct Script : mme::Layer {
        std::vector<std::function<void()>> steps;
        size_t frame = 0;
        Script() : Layer("Script") {}
        void OnUpdate(float) override {
            if (frame < steps.size()) steps[frame++]();
            else mme::Application::Get().Close();
        }
    };

    bool Contains(const std::string& entry) {
        for (const auto& e : g_Log) if (e == entry) return true;
        return false;
    }
}

static void TestLayerStackDuringRun() {
    g_Log.clear();
    mme::Application app(HiddenSpec());
    Script& script = app.PushLayer<Script>();
    Probe* a = nullptr;

    script.steps = {
        [&] {
            a = &app.PushLayer<Probe>("A");                 // döngü içinde: hemen eklenmez
            CHECK(!Contains("attach A"));
            app.PushOverlay<Probe>("Overlay");
        },
        [&] {
            CHECK(Contains("attach A"));                    // sonraki frame başında eklendi
            const auto& layers = app.GetLayers();
            CHECK(layers.size() == 3);
            CHECK(layers.back()->GetName() == "Overlay");   // overlay her zaman en üstte
            CHECK(layers[1]->GetName() == "A");             // oyun katmanı overlay'in altına
            app.PopLayer(*a);
            CHECK(!Contains("detach A"));                   // çıkarma da ertelenir
        },
        [&] {
            CHECK(Contains("detach A"));
            CHECK(app.GetLayers().size() == 2);
        },
    };
    app.Run();
    CHECK(script.frame == 3);
}

static void TestPauseStepAndTimeScale() {
    g_Log.clear();
    mme::Application app(HiddenSpec());
    Probe& probe = app.PushLayer<Probe>("P");
    Script& script = app.PushLayer<Script>();
    int before = 0;
    float lastDt = -1.0f, lastReal = -1.0f;

    // Ölçeklenmiş dt ile gerçek süreyi aynı frame'de yakalar
    struct DtProbe : mme::Layer {
        float* dtOut;
        float* realOut;
        DtProbe(float* d, float* r) : Layer("Dt"), dtOut(d), realOut(r) {}
        void OnUpdate(float dt) override { *dtOut = dt; *realOut = mme::Application::Get().GetFrameTime(); }
    };
    app.PushLayer<DtProbe>(&lastDt, &lastReal);

    script.steps = {
        [&] { app.SetPaused(true); },
        [&] { before = probe.fixedSteps; },
        [&] { CHECK(probe.fixedSteps == before); CHECK(lastDt == 0.0f); app.StepOneFrame(); }, // duraklatılmış: adım yok
        [&] { CHECK(probe.fixedSteps == before + 1); },                                        // tek adım
        [&] { app.SetPaused(false); app.SetTimeScale(2.0f); },
        [&] {},
        [&] {
            CHECK(lastReal > 0.0f && std::abs(lastDt - 2.0f * lastReal) < 1e-5f);           // dt ölçeklendi
        },
    };
    app.Run();
}

static void TestTwoApplicationsInSequence() {
    // Pencere/GLFW yaşam döngüsü: kapanıp yeniden açılabilmeli
    for (int i = 0; i < 2; ++i) {
        mme::Application app(HiddenSpec());
        Script& script = app.PushLayer<Script>();
        script.steps = { [] {} };
        app.Run();
        CHECK(script.frame == 1);
    }
}

int main() {
    mme::SetLogLevel(mme::LogLevel::Warn);
    struct { const char* name; void (*fn)(); } tests[] = {
        { "LayerStackDuringRun", TestLayerStackDuringRun },
        { "PauseStepAndTimeScale", TestPauseStepAndTimeScale },
        { "TwoApplicationsInSequence", TestTwoApplicationsInSequence },
    };
    int failed = 0;
    for (const auto& t : tests) {
        const int before = g_Failures;
        t.fn();
        const bool ok = g_Failures == before;
        failed += ok ? 0 : 1;
        std::printf("[%s] %s\n", ok ? " OK " : "FAIL", t.name);
    }
    std::printf("\n%d test, %d basarisiz\n", (int)(sizeof(tests) / sizeof(tests[0])), failed);
    return failed == 0 ? 0 : 1;
}
