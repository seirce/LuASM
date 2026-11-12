--!lorom

-- Automatically restore 1 energy every 60 gameplay frames.
-- This example intentionally does not use require() or the Energy library.
-- The healing instructions below are written out exactly as Energy.heal(1)
-- would emit them.

-- Replace the existing call to the room-shaking handler with our wrapper.
org("$828BAF")
jsl("AutoHealHook")

-- Put the custom routine in expanded ROM space.
org("$E08000")
routine("AutoHealHook", function()
    -- Save the incoming processor flags, then force 16-bit register
    -- widths so the pushes and pulls remain balanced after the original
    -- room-shaking routine changes the processor mode.
    php()
    rep("#$30")
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

    -- Inline expansion of Energy.heal(1):
    php()
    rep("#$30")
    lda("#$0001")
    jsl("$91DF12")
    plp()

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
