# Translation rules

LuASM reads one statement per line. The translator indexes function declarations, then expands calls in source order. The patch and each loaded library have their own function and import names.

## Functions, bindings and labels

- Define a template with `function name(parameters)` and close it with `end`. A template can call another template, including one declared later in the same file. Expansion cycles and wrong argument counts produce diagnostics.
- Plain parameters substitute into operands, including indexed and indirect addressing. Pass `#$0002` to a parameter to supply a complete immediate operand. Numeric tokens keep their spelling and width.
- Use `#Label` for a literal immediate label, or `##param .. "Suffix"` inside a template to construct one. Quoted strings in template arguments supply text for substitution. A direct string literal in a data directive retains its assembly quotes.
- `name = template(args)` labels one expansion. `name = do` labels an instruction block. Both emit a label on its own line and indent the body with one tab. `label("Name")` is also available.
- A scalar binding such as `tile = $004D` is available to subsequent operands and raw placeholders. Function parameters are scoped to their expansion. The optional `local` keyword is discarded.
- Luau runtime expressions, loops and callbacks are outside the translation language. Write processor control flow using instructions and labels.

## Library lookup

`beam = require("Beam Blocks")` associates the alias with a file named `Libraries/Beam Blocks.luau`. Lookup walks upward from the importing file, then from the working directory. An explicit path such as `require("\\\\helpers\\\\Beam")` is relative to the importing file. The extension is optional.

The first call through an alias loads and indexes that library. Only the called function bodies contribute ASM. Each nested call resolves imports in the file that defines the function. Libraries can expose plain functions or use the table form `local T = {}`, `function T.pointer(name)`, and `return T`.

## Layout

Blank lines, standalone comments and trailing comments carry through at their source positions. Function body comments and blank lines travel with each expansion. A trailing comment on a labeled call belongs to the label line. The root file sets the output newline style (LF or CRLF) and whether the final line has a terminator.

An `asm` block ends on its own `end` line. Its nonblank lines determine the common leading whitespace to remove. Relative indentation, trailing spaces, comment characters and opcode case pass through. Bound `<identifier>` placeholders insert their values verbatim. Unknown placeholders remain available to native assembler macros.

## Assembler targets

| Source | Asar 1.91 | xkas 0.06 |
| --- | --- | --- |
| `lda.w($0C18,X)` | `LDA.w $0C18,X` | `LDA.w $0C18,X` |
| `incbin("tiles.bin")` | `INCBIN "tiles.bin"` | `INCBIN tiles.bin` |
| `pushpc()`, `pullpc()`, `freecode()`, `freedata()` | Supported | Diagnostic |
| `assume(mx:off)`, `loadpc(...)`, `savepc(...)` | Diagnostic | Supported |
| `--!lorom`, `--!hirom` | Supported | Supported |

Generated mnemonics and data directives are uppercase. Mapping directives and `org` are lowercase. Width suffixes use `.b`, `.w` or `.l`. Include paths resolve relative to the output patch, which sits next to the input file. Run xkas from the patch directory for relative includes. Its unquoted include operands require paths without spaces.

The assembler resolves labels and validates addressing modes. Use raw ASM for native constructs without a LuASM method. Asar syntax is documented in the [Asar 1.9 manual](https://rpghacker.github.io/asar/asar_19/manual/). The xkas 0.06 distribution includes byuu's version-specific `xkas.html` manual.
