--!lorom

-- Ordinary Luau functions are compile-time helpers.
-- Calling one emits its instructions inline at the call site.
function LoadPlayerData()
    lda("$09A2")
    sta("$12")
end

org("$82F000")
LoadPlayerData()

-- A routine emits a named Asar label and its assembly body.
-- The callback runs during compilation; the generated code runs in-game.
jsl("SaveGame")

routine("SaveGame", function()
    php()
    phb()
    lda("$09A2")
    sta("$12")
    plb()
    plp()
    rtl()
end)
