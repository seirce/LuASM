# Tests

Compiler regression tests will live here.

The initial smoke test should compile `examples/first_patch.luau` and verify that the generated assembly contains:

```asm
lorom
org "$82E833"
lda $07B3
bit #$0008
bne $24
```

The exact Asar formatting may be refined as the emitter and golden-file tests are implemented.
