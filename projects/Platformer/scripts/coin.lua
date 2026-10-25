-- Dönen altın: oyuncu değince toplanır.
properties = {
    bobHeight = 5,
}

function OnStart(self)
    self.game = Scene.Find("Oyun")
    if self.game and self.game.script then self.game.script:RegisterCoin() end
    self.baseY = self.entity.y
    self.t = self.entity.x * 0.01   -- her altın farklı fazda sallansın
end

function OnUpdate(self, dt)
    self.t = self.t + dt
    self.entity.y = self.baseY + math.sin(self.t * 3) * self.bobHeight
end

function OnTrigger(self, other)
    if self.taken or other.name ~= "Oyuncu" then return end
    self.taken = true
    if self.game and self.game.script then self.game.script:AddCoin(self.entity.position) end
    self.entity:Destroy()
end
