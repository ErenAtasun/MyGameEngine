-- Haritanın altı: düşen oyuncu başa döner, düşen diğer nesneler silinir.
function OnTrigger(self, other)
    if other.name == "Oyuncu" then
        other.script:Respawn()
    else
        other:Destroy()
    end
end
