-- Oyuncu: A/D ya da oklarla koşar, Boşluk/W/Yukarı ile zıplar.
-- Zemin kontrolü ayakların altına atılan ışınlarla (Physics.Raycast) yapılır.
properties = {
    speed = 320,          -- yatay hız (px/s)
    jumpSpeed = 760,      -- zıplama hızı
    coyoteTime = 0.1,     -- kenardan düştükten sonra hâlâ zıplanabilen süre
    jumpBuffer = 0.12,    -- yere değmeden hemen önce basılan zıplamayı hatırla
}

local function jumpHeld()
    return Input.KeyDown("Space") or Input.KeyDown("W") or Input.KeyDown("Up")
end

local function jumpPressed()
    return Input.KeyPressed("Space") or Input.KeyPressed("W") or Input.KeyPressed("Up")
end

function OnStart(self)
    self.spawn = self.entity.position
    self.coyote = 0
    self.buffer = 0
    self.facing = 1
    self.wasGrounded = true
end

function IsGrounded(self)
    local p = self.entity.position
    local box = self.entity.box
    local half = box.size * 0.5
    local feet = p.y + box.offset.y - half.y
    for _, dx in ipairs({ -half.x + 3, 0, half.x - 3 }) do
        local hit = Physics.Raycast(vec2(p.x + dx, feet + 2), vec2(p.x + dx, feet - 6), self.entity)
        if hit then return true end
    end
    return false
end

function OnUpdate(self, dt)
    local body = self.entity.body
    local v = body.velocity
    local move = math.clamp(Input.Axis("A", "D") + Input.Axis("Left", "Right"), -1, 1)
    v.x = move * self.speed

    local grounded = self:IsGrounded()
    if grounded then self.coyote = self.coyoteTime else self.coyote = self.coyote - dt end
    if jumpPressed() then self.buffer = self.jumpBuffer else self.buffer = self.buffer - dt end

    if self.buffer > 0 and self.coyote > 0 then
        v.y = self.jumpSpeed
        self.coyote = 0
        self.buffer = 0
        Audio.Play("sounds/jump.wav", 0.5)
    end
    -- Tuş erken bırakılırsa daha alçak zıpla
    if v.y > 0 and not jumpHeld() then v.y = v.y - 3000 * dt end
    body.velocity = v

    -- Yere inince toz
    if grounded and not self.wasGrounded then self.entity.particles:Burst(10) end
    self.wasGrounded = grounded

    -- Animasyon ve yön (sprite boyutunun işareti yatay çevirir)
    if move ~= 0 then self.facing = move end
    local sprite = self.entity.sprite
    local size = sprite.size
    size.x = math.abs(size.x) * self.facing
    sprite.size = size

    local anim = self.entity.animator
    if not grounded then anim:Play(3, 1, 1)
    elseif move ~= 0 then anim:Play(1, 2, 10)
    else anim:Play(0, 1, 1) end
end

function Respawn(self)
    Audio.Play("sounds/hurt.wav", 0.6)
    self.entity.position = self.spawn
    self.entity.body.velocity = vec2(0, 0)
end
