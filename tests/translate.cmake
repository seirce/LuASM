cmake_minimum_required(VERSION 3.20)
set(dir "${CMAKE_CURRENT_BINARY_DIR}/translation-fixtures")
file(MAKE_DIRECTORY "${dir}/Libraries")
file(WRITE "${dir}/Libraries/PLM templates.luau" [=[local T = {}
local unused = require("absent")
local pointer = require("pointer")
function T.make(name)
    pointer.word(name)
end
function T.unused()
    nop()
end
return T
]=])
file(WRITE "${dir}/Libraries/pointer.luau" [=[function word(name)
    dw(##name .. "PLM_Pre", $CBB7)
end
]=])
file(WRITE "${dir}/beam.luau" [=[--!lorom

org($84F205) -- write to $84F205
local beam = require("PLM templates")
IcePLM = beam.make("Ice")
SpazerPLM = beam.make("Spazer")
PlasmaPLM = do
    dw(#PlasmaPLM_Pre, $CBB7)
end
HyperPLM = pointer("Hyper")
function pointer(name)
    asm
        DW #<name>PLM_Pre, $CBB7
    end
end

--- pre-instructions
org($84F105)
IcePLM_Pre = do
    lda.w($0C2C,X)
    cmp.w(#1000)
    rts()
end
asm
    ;WavePLM:
    ;    DW #WavePLM_Pre, $CBB7
    SpazerPLM_Pre:
        lda #$0001   ; exact case
    PlasmaPLM_Pre:
    HyperPLM_Pre:
        RTS
end
]=])
set(expected [=[lorom

org $84F205 ; write to $84F205
IcePLM:
	DW #IcePLM_Pre, $CBB7
SpazerPLM:
	DW #SpazerPLM_Pre, $CBB7
PlasmaPLM:
	DW #PlasmaPLM_Pre, $CBB7
HyperPLM:
	DW #HyperPLM_Pre, $CBB7

;; pre-instructions
org $84F105
IcePLM_Pre:
	LDA.w $0C2C,X
	CMP.w #1000
	RTS
;WavePLM:
;    DW #WavePLM_Pre, $CBB7
SpazerPLM_Pre:
    lda #$0001   ; exact case
PlasmaPLM_Pre:
HyperPLM_Pre:
    RTS
]=])
function(check tool stem expected)
    execute_process(COMMAND "${tool}" "${dir}/${stem}.luau" RESULT_VARIABLE status ERROR_VARIABLE err)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "${err}")
    endif()
    file(READ "${dir}/${stem}.asm" actual HEX)
    if(WIN32 AND NOT stem STREQUAL "layout")
        string(REPLACE "\n" "\r\n" expected "${expected}")
    endif()
    string(HEX "${expected}" wanted)
    if(NOT actual STREQUAL wanted)
        file(READ "${dir}/${stem}.asm" readable)
        message(FATAL_ERROR "Unexpected ${stem}.asm from ${tool}:\n${readable}\nExpected:\n${expected}")
    endif()
endfunction()
function(reject tool source diagnostic)
    file(WRITE "${dir}/invalid.luau" "${source}")
    file(WRITE "${dir}/invalid.asm" "previous patch")
    execute_process(COMMAND "${tool}" "${dir}/invalid.luau" RESULT_VARIABLE status ERROR_VARIABLE err)
    file(READ "${dir}/invalid.asm" previous)
    if(status EQUAL 0 OR NOT err MATCHES "${diagnostic}" OR NOT previous STREQUAL "previous patch")
        message(FATAL_ERROR "Expected rejection for ${source}, got ${status} ${err}")
    endif()
endfunction()
file(WRITE "${dir}/asset.bin" "AB")
file(WRITE "${dir}/asset.luau" "incbin(\"asset.bin\")\n")
set(crlf "\r\n")
if(WIN32)
    set(crlf "\n") # file(WRITE) uses native text mode.
endif()
file(WRITE "${dir}/layout.luau" "---- title${crlf} \t${crlf}addr = $0DDE${crlf}asm -- block${crlf}\tlda <addr>  ; tail  ${crlf}end -- close${crlf}db(\"a,--b\", $01)")
foreach(tool IN ITEMS "${ASAR}" "${XKAS}")
    check("${tool}" beam "${expected}")
    check("${tool}" layout ";;; title\r\n \t\r\n; block\r\nlda $0DDE  ; tail  \r\n; close\r\nDB \"a,--b\", $01")
    reject("${tool}" "lda(##name)" "function parameter")
    reject("${tool}" "lda(\"#$00\")" "without quotes")
    reject("${tool}" "function loop()\nloop()\nend\nloop()" "recursive template")
    reject("${tool}" "function f(x)\nnop()\nend\nf()" "argument count")
    reject("${tool}" "A = do\nnop()" "missing end")
endforeach()
check("${ASAR}" asset "INCBIN \"asset.bin\"\n")
check("${XKAS}" asset "INCBIN asset.bin\n")
reject("${XKAS}" "pushpc()" "selected target")
reject("${ASAR}" "assume(mx:off)" "selected target")
