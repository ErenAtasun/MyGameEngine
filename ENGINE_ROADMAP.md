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

## Aşama 5 — Oyun yapım motoru: editör + Lua + Box2D ✅
Amaç: kod yazmadan (yalnızca sahne + Lua) oyun yapılabilen, Unity/Godot benzeri editörlü bir 2D motor.

### 5C — Fizik: Box2D 3.1
- [x] `Physics2D` Box2D üstüne yeniden yazıldı; component'ler veri olarak kaldı (kod `velocity`/`position` yazar, bir sonraki adımda uygulanır)
- [x] Piksel birimleri (`b2SetLengthUnitsPerMeter(100)`), rotasyon geri yazımı tam trigonometriyle (Box2D yaklaşımları kayma yapıyordu)
- [x] `Collider2D` taban sınıfı (offset, tetik, yoğunluk, sürtünme, sekme) + `BoxCollider2D` (döner) + `CircleCollider2D`
- [x] `RigidBody2D`: açısal hız, `fixedRotation`, `bullet`, `AddForce`, kütle
- [x] Sürekli çarpışma (tünelleme yok), `Physics2D::Raycast`, tetik/temas olayları, uyuyan gövdeler
- [x] Ping-Pong yeni malzeme API'sine taşındı

### 5B — Lua script'leri (sol2)
- [x] `ScriptComponent`: sahne başına bir Lua VM, script başına ayrı ortam; `OnStart/OnUpdate/OnFixedUpdate/OnCollision/OnTrigger/OnUI/OnDestroy`
- [x] Entity/component'ler silinmeye karşı güvenli tanıtıcılarla (`EntityRef`); silinmiş entity'ye erişim anlaşılır Lua hatası verir
- [x] API: `vec2`, `Color`, `Scene` (Find/Create/Spawn/Load), `Input` (Axis, MouseWorld), `Physics` (Raycast), `Audio`, `UI`, `Time`, `Log`
- [x] `properties` tablosu inspector'da düzenlenir ve sahneye kaydedilir; oyun çalışırken canlı değerler düzenlenir
- [x] Hot-reload (durum korunur), hatalar `dosya:satır` ile konsola düşer, hatalı script düzeltilince kaldığı yerden devam eder
- [x] Prefab'lar (`*.prefab.json`): editörde "Prefab olarak kaydet", Lua'da `Scene.Spawn`

### 5A — Editör (`Editor.exe`)
- [x] ImGui docking: Sahne, Hiyerarşi, Inspector, Dosyalar, Konsol, Ayarlar; ilk açılışta varsayılan düzen
- [x] Framebuffer'a çizilen viewport, editör kamerası (sağ/orta tuş kaydır, tekerlek imlece doğru yakınlaş), ızgara
- [x] Tıklayarak seçim (dönmüş sprite/collider), Taşı/Döndür/Ölçekle gizmo'su (W/E/R), Ctrl ile ızgaraya/15°'ye oturma
- [x] Geri al / yinele (sahne anlık görüntüsü; bir sürükleme = bir adım), çoğalt, sil, hiyerarşide sürükleyerek sıralama
- [x] Oynat / Duraklat / Adım / Durdur: oyun sahnenin kopyasında çalışır, Durdur'da her şey geri gelir
- [x] Dosya tarayıcı: resim → sahneye sürükle = sprite, script → entity'ye bağla, sahne/prefab aç; yeni script şablonu
- [x] Hazır nesneler (Sprite, Kutu, Top, Zemin, Tetik, Kamera, Parçacık), `CameraComponent`, sahne yerçekimi ayarı

### 5D — Proje ve oynatıcı
- [x] `project.json` (`mme::Project`), proje seçici / yeni proje oluşturma; asset'ler önce projede, sonra motorda aranır
- [x] `Player.exe <proje> [--scene ...]`, editörden "Oynatıcıda çalıştır" (F6)
- [x] "Oyunu dışa aktar": `<Proje>_Build/` içine oyun adıyla .exe + DLL'ler + motor asset'leri + `game/`

### 5E — Örnek projeler (C++ yok, yalnızca sahne + Lua)
- [x] `projects/Platformer`: koşma/zıplama (raycast zemin kontrolü, coyote time), itilebilir kasalar, yuvarlanan toplar, tahterevalli, altınlar, bayrak, HUD
- [x] `projects/Pong`: Ping-Pong'un Lua sürümü (raket açısı, hızlanma, skor, kazanan ekranı)
- [x] Testler: Box2D fiziği (rotasyon, daire, tünelleme, raycast, kod değişikliklerinin aktarımı) ve Lua (yaşam döngüsü, properties, hata + hot-reload, olaylar, prefab) — 35 GPU'suz + 3 pencereli test

## Aşama 6 — Sıradaki adımlar
- [ ] Linux / macOS doğrulaması (dışa aktarma ve "Oynatıcıda çalıştır" şu an Windows'a özel)
- [ ] Ebeveyn/çocuk hiyerarşisi (dönüşümler şu an düz)
- [ ] Eklemler (menteşe, mesafe, yay) ve tilemap'in sahne/editör desteği
- [ ] Çoklu seçim, prefab örneklerinin prefab'a bağlı kalması
- [ ] Sahne kamerasının editörde önizleme penceresi

## Bilinen sınırlamalar
- Fizik birimi piksel (100 px = 1 m): en kararlı aralık 10-1000 px boyutundaki nesneler. Static/Kinematic
  gövdeler kendi aralarında çarpışmaz (Box2D kuralı), en az biri Dynamic olmalı.
- Ses konumsal değil (2D panning / 3D ses yok).
- Hot-reload dokular ve Lua script'leri için; yerleşik shader motora gömülü.
- Editörde geri alma tüm sahnenin JSON anlık görüntüsüyle yapılır (küçük/orta sahneler için uygun).
- Lua'da component'ler her erişimde entity'den yeniden bulunur; `io`/`os` kütüphaneleri açık değil.
- Debug panelinde (F1) sahne *yükleme* yok; editör ve oynatıcı kendi sahnelerini yükler.
- TTF fontlar ASCII + Latin-1 + Latin Extended-A ile sınırlı (Türkçe tam; Kiril, Yunan, CJK yok).
  Yerleşik 5x7 font yalnızca ASCII, Türkçe harfleri en yakın karşılığına çevirir.
- Çalışma dizinini .exe klasörüne alma, oynatıcıyı başlatma ve dışa aktarma yalnızca Windows'ta.
