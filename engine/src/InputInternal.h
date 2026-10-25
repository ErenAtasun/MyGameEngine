#pragma once
#include <MME/Event.h>

// Yalnızca motorun kullandığı Input kancaları (oyun koduna açık değil).
namespace mme::Input::Internal {
    void BeginFrame();                  // olaylar işlenmeden ÖNCE çağrılır
    void ProcessEvent(const Event& e);
}
