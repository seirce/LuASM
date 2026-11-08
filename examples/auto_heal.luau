--!lorom

-- Automatically restore 1 energy every 60 gameplay frames.
local Energy = require("Energy")

-- Replace the existing call to the room-shaking handler with our wrapper.
-- The wrapper calls the original handler, then checks the game timer.
org("$828BAF")
jsl("AutoHealHook")

-- Put the custom routine in expanded ROM space.
org("$E08000")
routine("AutoHealHook", function()
    -- Preserve the registers and processor flags expected by the game.
    php()
    pha()
    phx()
    phy()

    -- Run the original instruction that was replaced by our hook.
    jsl("$A08687")

    -- $09DA is the in-game frame counter (0 through 59).
    -- The game's timer resets it to zero once every 60 gameplay frames.
    lda("$09DA")
    cmp("#$0000")
    bne("AutoHealDone")

    -- Preserve DP_Temp12, which the vanilla healing routine uses as scratch.
    lda("$12")
    pha()
    Energy.heal(1)
    pla()
    sta("$12")

    -- Restore the original machine state and return to the game loop.
    label("AutoHealDone")
    ply()
    plx()
    pla()
    plp()
    rtl()
end)
