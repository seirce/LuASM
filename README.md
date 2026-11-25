# LuASM

LuASM translates Luau-shaped Super Metroid patches into 65c816 assembly. Choose `LuASM-asar` for Asar 1.91 or `LuASM-xkas` for xkas 0.06. Both write a sibling `.asm` file that you can inspect and assemble with your usual patching tools. The original `LuASM` command selects Asar.

## Build and run

Build with CMake 3.20 or newer and a C++17 compiler. From the repository root on Windows with Visual Studio:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\LuASM-asar.exe examples\first_patch.luau
```

For xkas, run `.\build\Release\LuASM-xkas.exe examples\first_patch.luau`. Both commands write `examples\first_patch.asm`. With a single-configuration generator such as Ninja, the executables are directly under `build`.

## Write a patch

```lua
--!lorom

org($84F205) -- beam block table
function orgPLM(name)
    dw(##name .. "PLM_Pre", $CBB7)
end
IcePLM = orgPLM("Ice")
SpazerPLM = do
    dw(#SpazerPLM_Pre, $CBB7)
end
```

Both definitions produce a label followed by an indented instruction.

```asm
lorom

org $84F205 ; beam block table
IcePLM:
	DW #IcePLM_Pre, $CBB7
SpazerPLM:
	DW #SpazerPLM_Pre, $CBB7
```

A function expands wherever you call it. A plain call inserts its body inline. Assigning a call gives that expansion a label. To emit a machine-code call, write `jsr(MyLabel)` or `jsl(MyLabel)` and provide the matching return in the patch.

Use bare assembly operands such as `lda(#$0000)`, `beq(ICEAlpha)` and `lda.w($0C2C,X)`. A single `#` keeps its assembly meaning. Inside a function, `##name` prefixes the parameter value with a literal `#`, and `..` joins the surrounding pieces. Asar 1.91 accepts `DW #Label` with a deprecation warning. To emit `DW Label`, use `dw(name .. "PLM_Pre", $CBB7)`.

## Libraries and raw assembly

In `Libraries/Beam.luau`, write the functions the patch will reference.

```lua
function pointer(name)
    asm
        DW #<name>PLM_Pre, $CBB7
    end
end
```

Then use the library from a patch:

```lua
beam = require("Beam")
org($84F205)
IcePLM = beam.pointer("Ice")
```

Library functions can call other functions and require other libraries. A leading `local` is optional. Inside an `asm` block, LuASM removes the shared indentation margin and substitutes bound `<name>` placeholders. Write the remaining text exactly as the selected assembler expects, including capitalization, comments and spacing.

Outside raw ASM, blank lines pass through and each extra dash in a comment adds a semicolon. For example, `--- pre-instructions` becomes `;; pre-instructions`.

See [translation rules and target differences](docs/COMPILER_DESIGN.md) for the supported syntax.
