-- Kamera hedefi yumuşakça takip eder.
properties = {
    target = "Oyuncu",
    smooth = 5,           -- büyüdükçe daha sıkı takip
    offset = vec2(0, 80),
    minY = -120,          -- kamera bundan aşağı inmez (zeminin altı görünmesin)
}

function OnUpdate(self, dt)
    if not (self.follow and self.follow.valid) then
        self.follow = Scene.Find(self.target)
        if not self.follow then return end
    end
    local goal = self.follow.position + self.offset
    goal.y = math.max(goal.y, self.minY)
    local k = 1 - math.exp(-self.smooth * dt)
    self.entity.position = self.entity.position:lerp(goal, k)
end
