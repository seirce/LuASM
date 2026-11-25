--!lorom

-- Automatically restore 1 energy every 60 gameplay frames.
function heal(amount)
    php()
    rep(#$30)
    lda(amount)
    jsl($91DF12)
    plp()
end

-- Replace the existing call to the room-shaking handler with our wrapper.
-- The wrapper calls the original handler, then checks the game timer.
org($828BAF)
jsl(AutoHealHook)

-- Put the custom routine in expanded ROM space.
org($E08000)
AutoHealHook = do
    -- Preserve the registers and processor flags expected by the game.
    php()
    rep(#$30)
    pha()
    phx()
    phy()

    -- Run the original instruction that was replaced by our hook.
    jsl($A08687)

    -- $09DA is the in-game frame counter (0 through 59).
    -- The game's timer resets it to zero once every 60 gameplay frames.
    lda($09DA)
    cmp(#$0000)
    bne(AutoHealDone)

    -- Preserve DP_Temp12, which the vanilla healing routine uses as scratch.
    lda($12)
    pha()
    heal(#$0001)
    pla()
    sta($12)

    -- Restore the original machine state and return to the game loop.
    label(AutoHealDone)
    ply()
    plx()
    pla()
    plp()
    rtl()
end
