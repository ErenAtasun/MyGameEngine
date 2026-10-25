-- Maç yöneticisi: servis, skor, kazanan ekranı.
properties = {
    winScore = 5,
}

function OnStart(self)
    self.score = { 0, 0 }
    self.state = "serve"      -- serve | play | over
    self.serveDir = 1
    self.ball = Scene.Find("Top")
end

function Goal(self, player)
    if self.state ~= "play" then return end
    self.score[player] = self.score[player] + 1
    self.ball.script:Reset()
    self.serveDir = player == 1 and 1 or -1   -- yiyen taraf servis alır
    if self.score[player] >= self.winScore then
        self.state = "over"
        self.winner = player
        Audio.Play("sounds/win.wav", 0.8)
    else
        self.state = "serve"
        Audio.Play("sounds/score.wav", 0.6)
    end
end

function OnUpdate(self, dt)
    if self.state == "serve" and Input.KeyPressed("Space") then
        self.ball.script:Launch(self.serveDir)
        self.state = "play"
    elseif self.state == "over" and (Input.KeyPressed("Space") or Input.KeyPressed("R")) then
        Scene.Load("scenes/main.json")
    end
end

function OnUI(self)
    local w, h = UI.Width(), UI.Height()
    UI.TextCentered(tostring(self.score[1]), w * 0.25, 70, 72, Color(0.45, 0.95, 0.55, 0.9))
    UI.TextCentered(tostring(self.score[2]), w * 0.75, 70, 72, Color(1, 0.45, 0.45, 0.9))
    if self.state == "serve" then
        UI.TextCentered("Boşluk: servis    W/S - Yukarı/Aşağı", w / 2, h - 40, 22, Color(1, 1, 1, 0.6))
    elseif self.state == "over" then
        UI.Rect(0, h / 2 - 70, w, 140, Color(0, 0, 0, 0.6))
        local name = self.winner == 1 and "Sol oyuncu" or "Sağ oyuncu"
        UI.TextCentered(name .. " kazandı!", w / 2, h / 2 - 16, 48, Color(1, 0.85, 0.3, 1))
        UI.TextCentered("Boşluk: yeni maç", w / 2, h / 2 + 36, 22, Color(1, 1, 1, 0.85))
    end
end
