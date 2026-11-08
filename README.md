# LuASM

LuASM is a Luau-shaped front end for writing Super Metroid 65c816 assembly patches. It generates Asar `.asm` files; it does not patch ROMs. You can use this for other SNES games, however, you will need to write your own libraries for that.

**Status: early prototype / not release-ready.** The compiler embeds Luau and currently exposes an initial set of lowercase 65c816 instruction-emitter functions, plus `org()`, `label()`, `routine()`, and `incbin()`.

## Build

Requirements:
- CMake 3.20 or newer
- A C++17 compiler (Visual Studio 2022 Build Tools on Windows)
- Git (CMake uses it to fetch the Luau source)

From the repository root:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

The executable will be produced under the build directory (the exact configuration subfolder depends on the generator).

## Run

```powershell
.\build\Release\LuASM.exe examples\first_patch.luau
```

This writes `examples\first_patch.asm` next to the input file.

## Assembly routine example

Ordinary Luau functions are compile-time helpers: calling one emits its assembly instructions inline. Use `routine(name, function)` to define a named assembly routine that can be called with `jsr()` or `jsl()`:

```lua
--!lorom

org("$82F000")
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
```

The `routine` callback runs during compilation to emit the routine body. The generated label and machine instructions are executed in-game when control flow reaches them. LuASM does not automatically add `rts()` or `rtl()`; choose the appropriate return instruction yourself.

## Libraries

`require("Energy")` loads `Libraries/Energy.luau` and returns the module value. Library files are compiled and executed in the same Luau VM as the input script, and each module is cached for the duration of that compilation. A require string beginning with a backslash, such as `require("\\helpers\\MyLibrary")`, instead resolves a `.luau` file relative to the directory containing the input `.luau` file. The compiler does not contain game-specific APIs; game behavior belongs in library modules.

The initial `Energy` library targets the unmodified Super Metroid game. Its `current` and `max` fields are RAM address strings, while `heal()` and `damage()` emit calls to the game's built-in health routines.

See [Compiler design](docs/COMPILER_DESIGN.md) for the semantics and current limitations.

## Basic instruction example

```lua
--!lorom

org("$82E833")
lda("$07B3")
bit("#$0008")
bne("$24")

org("$90EA57")
nop()
nop()
nop()
```

## Current limitations

Instruction calls currently emit mnemonic text and do not validate addressing modes. The full instruction catalogue, addressing-mode validation, runtime callbacks, richer Asar directives, and comprehensive tests remain to be implemented.

Luau is built into LuASM as a dependency; a separate `luau.exe` is not required. CMake fetches Luau source during configuration, so the first configure step requires an internet connection.
