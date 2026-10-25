-- Raket: iki tuşla yukarı/aşağı. Kinematik gövde hızla sürülür, böylece top
-- çarpınca fizik doğru sekmeyi hesaplar.
properties = {
    up = "W",
    down = "S",
    speed = 560,
    limit = 300,    -- merkezden en fazla bu kadar uzaklaşır
}

function OnStart(self)
    self.isPaddle = true
end

function OnFixedUpdate(self, step)
    local dir = 0
    if Input.KeyDown(self.up) then dir = dir + 1 end
    if Input.KeyDown(self.down) then dir = dir - 1 end
    local y = self.entity.y
    -- Hız, raket sınırda tam duracak şekilde seçilir (taşıp geri çekilmez)
    local target = math.clamp(y + dir * self.speed * step, -self.limit, self.limit)
    self.entity.body.velocity = vec2(0, (target - y) / step)
end
