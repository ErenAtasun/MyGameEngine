-- Oyun yöneticisi: altın sayacı, süre, bitiş ekranı. Diğer script'ler
-- Scene.Find("Oyun").script üzerinden buradaki fonksiyonları çağırır.
properties = {
    restartKey = "R",
}

function OnStart(self)
    self.coins = 0
    self.totalCoins = 0
    self.time = 0
    self.won = false
end

function RegisterCoin(self) self.totalCoins = self.totalCoins + 1 end

function AddCoin(self, position)
    self.coins = self.coins + 1
    Audio.Play("sounds/coin.wav", 0.6)
    Scene.Spawn("prefabs/sparkle.prefab.json", position)
end

function Win(self)
    if self.won then return end
    self.won = true
    Audio.Play("sounds/win.wav", 0.8)
end

function OnUpdate(self, dt)
    if not self.won then self.time = self.time + dt end
    if Input.KeyPressed(self.restartKey) then Scene.Load("scenes/main.json") end
end

function OnUI(self)
    UI.Rect(16, 16, 250, 76, Color(0, 0, 0, 0.45))
    UI.Text("Altın: " .. self.coins .. " / " .. self.totalCoins, 30, 26, 26, Color(1, 0.85, 0.3, 1))
    UI.Text(string.format("Süre: %.1f sn", self.time), 30, 60, 20, Color(1, 1, 1, 0.9))

    if self.won then
        local w, h = UI.Width(), UI.Height()
        UI.Rect(0, h / 2 - 80, w, 160, Color(0, 0, 0, 0.6))
        UI.TextCentered("Tebrikler! Bölüm tamamlandı", w / 2, h / 2 - 28, 44, Color(0.55, 1, 0.6, 1))
        UI.TextCentered(string.format("%d altın, %.1f saniye   -   %s: yeniden başla", self.coins, self.time, self.restartKey),
                        w / 2, h / 2 + 30, 22, Color(1, 1, 1, 0.9))
    end
end
