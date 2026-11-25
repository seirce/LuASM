--!lorom

-- Ordinary Luau functions are compile-time helpers.
-- Calling one emits its instructions inline at the call site.
function LoadPlayerData()
    lda($09A2)
    sta($12)
end

org($82F000)
LoadPlayerData()

-- A named block defines the destination for JSL.
jsl(SaveGame)

SaveGame = do
    php()
    phb()
    lda($09A2)
    sta($12)
    plb()
    plp()
    rtl()
end
