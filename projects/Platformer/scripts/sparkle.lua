-- Altın toplanınca beliren parıltı (prefabs/sparkle.prefab.json): patlar ve kendini siler.
properties = {
    count = 24,
    life = 0.8,
}

function OnStart(self)
    self.entity.particles:Burst(self.count)
end

function OnUpdate(self, dt)
    self.life = self.life - dt
    if self.life <= 0 then self.entity:Destroy() end
end
