# LuASM compiler design

## Goal

LuASM source is Luau syntax used as a readable surface for Asar assembly. Assembly methods such as `lda`, `sta`, and `bne` emit their corresponding 65c816 mnemonics. LuASM writes an `.asm` file; Asar and the user's ROM editor remain responsible for applying the patch.

## Current implementation

The compiler embeds the Luau VM and compiler libraries into the LuASM executable. It does not launch a separate `luau.exe`. CMake fetches Luau's source and builds the required libraries.

The current prototype supports:
- `--!lorom` as a file-level directive.
- The initial 65c816 mnemonic set as lowercase global functions.
- `org(address)` for Asar origin directives.
- `label(name)` for explicit Asar labels.
- `routine(name, function)` for named assembly routine definitions.
- `incbin(path)` for existing binary files next to the Luau source file.
- Output to a sibling `.asm` file.

Instruction operands are passed as strings, e.g. `lda("$7E1234")` and `rep("#$30")`. Zero-operand instructions use calls such as `nop()`.

## Compile-time helpers and assembly routines

There are two distinct kinds of Luau functions:

1. **Compile-time helper:** an ordinary Luau function runs when called by the compiler. Any assembly-emitter calls inside it are written at the point where the helper is called.
2. **Assembly routine:** `routine(name, callback)` writes an Asar label and executes the callback to emit the routine body at that location. It does not automatically add a return instruction or call the routine.

Example:

```lua
function LoadPlayerData()
    lda("$09A2")
    sta("$12")
end

org("$82F000")
LoadPlayerData() -- Emits these instructions inline.

jsl("SaveGame") -- Emits a machine-code call to the routine.

routine("SaveGame", function()
    php()
    phb()
    ; assembly instructions go here
    plb()
    plp()
    rtl()
end)
```

The example's semicolon comment is explanatory Asar-style pseudocode; LuASM's comment preservation is not implemented yet. Use valid Luau comments (`--`) in actual source files.

A routine callback is executed during compilation to collect its assembly text. The resulting label and instructions are what execute in the game when reached by 65c816 control flow. The callback itself is not a runtime callback. You must explicitly emit `rts()`, `rtl()`, or another appropriate ending instruction; choose it according to how the routine is called and the calling convention.

Current constraints:
- `routine` requires exactly a non-empty label name and a function.
- Routine names cannot contain whitespace or a colon.
- Nested `routine` declarations are rejected.
- The compiler does not yet validate duplicate labels, routine calling conventions, or whether `jsr`/ `jsl` matches the routine's return instruction.
- Ordinary Luau functions, loops, and conditionals execute at compile time; they do not automatically become in-game logic.

## Important work still required

1. A complete, checked instruction catalogue and addressing-mode validation are still required. The mnemonic functions currently emit text and do not validate whether an operand form is legal for a particular instruction.
2. Built-in and user library loading through `require()` is planned, but not implemented.
3. Runtime callbacks such as `run.repeat(frames, callback)` need a compiler-defined lowering strategy; they cannot simply run once during compilation.
4. Asar directives, comments, include search paths, source maps, duplicate-label checks, and robust diagnostics need more coverage.
5. Automated compiler tests and integration tests with Asar are still needed.

## Binary assets

The planned `incbin("file.gfx")` syntax emits Asar's `incbin "file.gfx"` directive. The compiler checks that the file exists relative to the source file but leaves the emitted path relative, so the generated patch should be kept with its asset or adjusted to a future configurable asset root.
