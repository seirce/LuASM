# LuASM compiler design

## Goal

LuASM source is Luau syntax used as a readable surface for Asar assembly. Assembly methods such as `lda`, `sta`, and `bne` emit their corresponding 65c816 mnemonics. LuASM writes an `.asm` file; Asar and the user's ROM editor remain responsible for applying the patch.

## Initial implementation

The first implementation embeds the Luau VM and Luau compiler libraries into the LuASM executable. It does not launch a separate `luau.exe`. CMake fetches Luau's source and builds the required libraries.

The current prototype supports:
- `--!lorom` as a file-level directive.
- The initial 65c816 mnemonic set as lowercase global functions.
- `org(address)` for Asar origin directives.
- `label(name)` for explicit assembly labels.
- `incbin(path)` for existing binary files next to the Luau source file.
- Output to a sibling `.asm` file.

Instruction operands are passed as strings, e.g. `lda("$7E1234")` and `rep("#$30")`. Zero-operand instructions use calls such as `nop()`.

## Important design work still required

This prototype is a bootstrap, not a complete compiler. In particular:

1. A complete, checked instruction catalogue and addressing-mode validation are still required. The mnemonic functions currently emit text and do not validate whether an operand form is legal for a particular instruction.
2. The syntax and semantics for named assembly routines must be finalized. A normal Luau function call currently executes its body while the script is being evaluated; it does not yet create an Asar subroutine label or automatically emit `jsr`.
3. The supported subset of compile-time Luau must be defined. Ordinary Luau variables and conditionals execute in the embedded VM; they do not automatically become runtime 65c816 logic.
4. Built-in and user library loading through `require()` is planned, but not implemented.
5. Asar directives, comments, include search paths, source maps, and robust diagnostics need more coverage.
6. The Luau dependency should be pinned to a tested commit or release before producing reproducible release builds.

## Function and routine semantics

We must distinguish three concepts before implementing user-facing libraries:
- A Luau helper function that runs during compilation and emits assembly.
- An assembly subroutine that becomes a label and is invoked with `jsr` or `jsl`.
- A runtime callback scheduled to execute in the game, which must be lowered into generated machine-code control flow.

They must not be conflated. In particular, `run.repeat(frames, callback)` will eventually need a compiler-defined lowering strategy; it cannot simply run the callback once during compilation.

## Binary assets

The planned `incbin("file.gfx")` syntax emits Asar's `incbin "file.gfx"` directive. The prototype checks that the file exists relative to the source file but leaves the emitted path relative, so the generated patch should be kept with its asset or adjusted to a future configurable asset root.
