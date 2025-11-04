# Ping-Pong Oyunu Geliştirme Planı

## Oyun Bileşenleri

### 1. Fizik Sistemi
- **AABBCollider Component**: Dikdörtgen collision detection
- **Velocity Component**: Hareket ve hız yönetimi
- **Collision Response**: Çarpışma sonrası yön değiştirme

### 2. Oyun Nesneleri
- **Paddle (Raket)**: 2 adet (sol/sağ veya üst/alt)
  - Input ile kontrol
  - Ekran sınırlarında hareket
  - Top ile çarpışma
- **Ball (Top)**: 
  - Otomatik hareket (velocity)
  - Paddle ve duvarlarla çarpışma
  - Açılı yansıma
  - Reset mekanizması (goal sonrası)
- **Walls (Duvarlar)**: 
  - Üst/alt sınırlar (yansıma)
  - Sol/sağ goal alanları (skor)

### 3. Oyun Mantığı
- **Score System**: Oyuncu skorları
- **Ball Reset**: Goal sonrası merkeze dönüş
- **Game State**: Oyun durumu yönetimi

### 4. UI
- Skor gösterimi (sol/sağ veya üst/alt)
- Oyun bitiş ekranı (opsiyonel)

## Geliştirme Sırası

1. ✅ AABB Collision Component
2. ✅ Velocity Component  
3. ✅ Paddle Controller
4. ✅ Ball Component
5. ✅ Wall/Goal Entities
6. ✅ Score System
7. ✅ GameScene Setup
8. ✅ main.cpp Integration

## Kontroller
- Sol Paddle: W/S veya ↑/↓
- Sağ Paddle: Up/Down arrow keys (veya I/K)
- Pause: Space (opsiyonel)

