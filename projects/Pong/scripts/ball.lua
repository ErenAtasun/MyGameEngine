-- Top: raketin neresine çarparsa o açıyla seker, her vuruşta hızlanır.
-- Duvar sekmeleri fizikten gelir (collider restitution = 1, sürtünme 0).
properties = {
    baseSpeed = 420,
    gain = 1.06,
    maxSpeed = 1000,
    maxAngle = 55,
}

function Launch(self, dir)
    local a = math.rad(math.random(-25, 25))
    self.entity.body.velocity = vec2(math.cos(a) * dir, math.sin(a)) * self.baseSpeed
    self.entity.particles.emitting = true
    Audio.Play("sounds/blip.wav", 0.6, 0.8)
end

function Reset(self)
    self.entity.position = vec2(0, 0)
    self.entity.body.velocity = vec2(0, 0)
    self.entity.particles.emitting = false
end

function OnCollision(self, other, normal)
    local s = other.script
    if s and s.isPaddle and math.abs(normal.x) > 0.5 then
        -- -1 (alt uç) .. 1 (üst uç)
        local hit = math.clamp((self.entity.y - other.y) / (other.box.size.y * 0.5), -1, 1)
        local a = math.rad(hit * self.maxAngle)
        local speed = math.min(self.entity.body.velocity:length() * self.gain, self.maxSpeed)
        self.entity.body.velocity = vec2(math.cos(a) * normal.x, math.sin(a)) * speed
        Audio.Play("sounds/blip.wav", 0.8, 0.9 + 0.6 * speed / self.maxSpeed)
    else
        Audio.Play("sounds/bump.wav", 0.5)
    end
end

function OnTrigger(self, other)
    local game = Scene.Find("Oyun").script
    if other.name == "Sol Kale" then game:Goal(2)
    elseif other.name == "Sağ Kale" then game:Goal(1) end
end
