-- Bitiş bayrağı
function OnTrigger(self, other)
    if other.name ~= "Oyuncu" then return end
    local game = Scene.Find("Oyun")
    if game and game.script then game.script:Win() end
end
