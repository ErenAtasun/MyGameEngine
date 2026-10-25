# MakeMyEngine — Motor Yol Haritası

Amaç: Ping-Pong'u taşıyan iskeleti, oyun kodundan ayrılmış, yeniden kullanılabilir gerçek bir 2D motora dönüştürmek.
Her aşamanın sonunda proje derlenir, çalıştırılır ve ekran görüntüsü + sentetik girdiyle doğrulanır.

## Aşama 0 — Repo hijyeni ✅
- [x] `.gitignore` (build/, .vs/, out/) ve derleme çıktılarını git takibinden çıkarma
- [x] Kaynak dosyaları UTF-8'e çevirme (CP1254 kurtarılabilenler + `�` olmuş yorumların yeniden yazılması)

## Aşama 1 — Hata düzeltmeleri ✅
- [x] **Input**: `NewFrame()` poll'dan hemen sonra çağrıldığı için `KeyPressed` / `leftPressed` hiçbir zaman okunamıyordu → butonlar hiç çalışmıyordu
- [x] UI butonu basışta *ve* bırakışta tetikleniyordu (çift tıklama)
- [x] Font tablosu 6 bitlik desenleri 5 bit okuyordu (bozuk harfler) → tam ASCII 5x7 font
- [x] Top paddle'ın içinde kalınca her frame yön değiştirip takılıyordu
- [x] Restart'taki 4 yedek yol + her frame `cout` spam'i kaldırıldı
- [x] `DrawScreenQuad` çift y-çevirmesi → skorlar altta, yazı baş aşağıydı
- [x] Kazanan yazısı banner'dan taşıyordu → metin ölçülüp ortalanıyor
- [x] Yeniden boyutlandırma: paddle'lar, top sınırları ve çıkış butonu güncellenmiyordu
- [x] `Entity` kendine `shared_ptr` tutuyordu → bellek sızıntısı
- [x] Renderer: batch dolunca texture slotu sıfırlanıyor ama quad eski `texIndex`'i kullanıyordu
- [x] İki ayrı 1x1 beyaz doku (boşa giden texture slotu), shader programının iki kez silinmesi
- [x] `GetCollisionNormal` null kontrolü
- [x] `/W4` uyarıları ve ölü kod (`CollisionSystem` derlenmiyordu, `basic.frag`, deferred render yolu)

## Aşama 2 — Motor çekirdeği ✅
- [x] **Motor / oyun ayrımı**: `engine/` statik kütüphane (`MME::Engine`) + `games/PingPong/` çalıştırılabilir
- [x] Oyun kodu OpenGL/GLFW görmez: public header'larda GL tipi yok, kendi `KeyCodes.h` (GLFW ile `static_assert`'li)
- [x] `Application` + `Window` + `Layer` yaşam döngüsü (OnAttach / OnFixedUpdate / OnUpdate / OnRender / OnUIRender / OnEvent)
- [x] Olay sistemi (pencere, klavye, fare) — `Input` olaylarla beslenir, global `g_ScreenW/H` kalktı, yüksek DPI'da fare koordinatı ölçekleniyor
- [x] **Sabit zaman adımlı döngü** (fizik 60 Hz, render değişken, 0.25 sn dt sınırı, simge durumunda CPU harcamıyor)
- [x] `Log` (seviyeli, zaman damgalı, renkli) + `MME_ASSERT`
- [x] Sahiplik modeli: Scene → `unique_ptr<Entity>` → `unique_ptr<Component>`, tip bazlı arama, ertelenmiş silme
- [x] **Fizik sistemi**: `RigidBody2D` (Static / Kinematic / Dynamic), AABB çözümleme, restitution, yerçekimi, `OnCollisionEnter` / `OnTriggerEnter`
- [x] Ortografik kamera: merkez tabanlı pozisyon, merkeze göre zoom, `ScreenToWorld`
- [x] Font atlası (karakter başına tek quad) + `MeasureText` / `TextCentered`, çok satır desteği
- [x] Yerleşik sprite shader'ı motora gömülü (çalışmak için dosya gerekmez)
- [x] Tek doku yön kuralı: tüm dokularda v=0 görüntünün altı; UI geçişi bunu kendisi çevirir
- [x] F3 debug overlay (FPS, frame süresi, draw call, quad, texture bind)
- [x] Ping-Pong yeni API'de: gerçek duvar/kale entity'leri, çarpma noktasına göre açılı sekme, vuruş başına hızlanma

## Aşama 3 — Örnekler ve araçlar ✅
- [x] Tilemap örneği (`games/TilemapDemo`): kamera, zoom, fareyle karo seçme/boyama
- [x] Tilemap düzeltmeleri: CSV'nin ilk satırı artık üstte, atlas satırları doğru yönde, yarım karo kayması giderildi, eksik hücreli satırlar dolduruluyor
- [x] `CMakePresets.json` (vcpkg toolchain) + yerel `CMakeUserPresets.json` desteği
- [x] Birim testleri (girdi, sahne, fizik, kamera, tilemap, font ölçümü) + `ctest` — 18 test
- [x] README güncellemesi

## Aşama 4 — Yeni özellikler ve sağlamlaştırma ✅
### 4A — Altyapı
- [x] vcpkg manifest (`vcpkg.json`): bağımlılıklar repoda tanımlı, `cmake --preset` hepsini kurar
- [x] Gömülü `stb_image.h` yerine vcpkg `stb` (stb_truetype dahil)
- [x] Katman yığını: `PushLayer` / `PopLayer` döngü sırasında güvenli
- [x] Dear ImGui entegrasyonu + `Layer::OnImGuiRender`, ImGui fareyi/klavyeyi kullanırken oyun girdisi bastırılır
- [x] Debug paneli (F1): sahne hiyerarşisi, entity inspector, collider çizimi, renderer istatistikleri

### 4B — İçerik özellikleri
- [x] Ses (miniaudio): efektler, akışlı müzik, ses grupları (master / sfx / music)
- [x] `SpriteAnimator`: sprite sheet klipleri, fps, döngü
- [x] Parçacık sistemi (havuzlu, renk/boyut geçişli)
- [x] TTF font (stb_truetype) + UTF-8: Türkçe karakterler (ç ğ ı ö ş ü İ Ğ Ş)

### 4C — Veri
- [x] Sahne serileştirme (JSON), genişletilebilir component kaydı
- [x] Doku hot-reload (dosya değişince aynı TextureID ile yeniden yükleme)

### 4D — Sağlamlık
- [x] Fizik broadphase (uzamsal hash) — O(n²) yerine
- [x] Hızlı cisimler için alt adımlama (tünellemeyi önler)
- [x] Yeni sistemlerin birim testleri (29 GPU'suz + 3 gizli pencereli Application testi)

### Örneklere yansıması
- [x] Ping-Pong: ses efektleri, vuruş/gol parçacıkları, Türkçe arayüz yazıları
- [x] TilemapDemo: animasyonlu sprite, TTF Türkçe yazı, sahne kaydet/yükle

## Aşama 5 — Gelecek
- [ ] Linux / macOS doğrulaması
- [ ] Döndürülmüş kutular / daire çarpıştırıcılar
- [ ] Sahne editörü (ImGui üzerinde sürükle-bırak)

## Bilinen sınırlamalar
- Fizik yalnızca eksen hizalı kutular (rotasyon yok). Alt adımlama en fazla 8 adım; bunu aşacak kadar
  hızlı ve ince nesneler hâlâ birbirinin içinden geçebilir.
- Ses konumsal değil (2D panning / 3D ses yok).
- Hot-reload yalnızca dokular için; yerleşik shader motora gömülü.
- Debug panelinde sahne *yükleme* yok: oyunlar entity işaretçileri tuttuğu için yükleme oyunun kendi
  kodundan yapılmalı (TilemapDemo F9 örneği).
- TTF fontlar ASCII + Latin-1 + Latin Extended-A ile sınırlı (Türkçe tam; Kiril, Yunan, CJK yok).
  Yerleşik 5x7 font yalnızca ASCII, Türkçe harfleri en yakın karşılığına çevirir.
- Çalışma dizinini .exe klasörüne alma yalnızca Windows'ta yapılıyor.
