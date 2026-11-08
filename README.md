# LuASM

LuASM is a Luau-shaped front end for writing Super Metroid 65c816 assembly patches. It generates Asar `.asm` files; it does not patch ROMs. You can use this for other SNES games, however, you will need to write your own libraries for that.

**Status: early prototype / not release-ready.** The current bootstrap embeds the Luau VM and exposes an initial set of lowercase 65c816 instruction-emitter functions, plus `org()`, `label()`, and `incbin()`.

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

## Example

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

## Important limitations

This is only the first compiler bootstrap. Instruction calls currently emit mnemonic text and do not yet validate addressing modes. The full instruction catalogue, proper assembly-routine declarations/calls, library loading, game-runtime callbacks, richer Asar directives, and comprehensive tests remain to be implemented. See [Compiler design](docs/COMPILER_DESIGN.md) for the current architecture and distinctions that still need to be settled.

Luau is built into LuASM as a dependency; a separate `luau.exe` is not required. CMake fetches Luau source during configuration, so the first configure step requires an internet connection.
