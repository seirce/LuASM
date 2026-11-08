-- Super Metroid energy helpers.
-- This module is game-specific; the compiler's require() loader is not.

local Energy = {}

-- Samus's current and maximum energy are 16-bit values in WRAM.
Energy.current = "$09C2"
Energy.max = "$09C4"

local function amountOperand(amount, functionName)
    assert(
        type(amount) == "number"
            and amount == math.floor(amount)
            and amount >= 0
            and amount <= 0xFFFF,
        "Energy." .. functionName .. " expects an integer from 0 to 65535"
    )

    return string.format("#$%04X", amount)
end

-- Restore health using Super Metroid's own health restoration routine.
-- The vanilla routine also transfers overflow into reserve energy.
function Energy.heal(healthToHeal)
    local operand = amountOperand(healthToHeal, "heal")

    php()
    rep("#$30")
    lda(operand)
    jsl("$91DF12")
    plp()
end

-- Deal damage using Super Metroid's vanilla damage routine.
-- This direct routine does not apply suit damage reduction or knockback.
function Energy.damage(healthToDamage)
    local operand = amountOperand(healthToDamage, "damage")

    php()
    rep("#$30")
    lda(operand)
    jsl("$91DF51")
    plp()
end

return Energy
