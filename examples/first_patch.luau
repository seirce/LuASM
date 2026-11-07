--!lorom

-- Example: a small patch expressed with Luau-shaped assembly calls.
org("$82E833")
lda("$07B3")
bit("#$0008")
bne("$24")

org("$90EA57")
nop()
nop()
nop()
