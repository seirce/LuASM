--!lorom

--pointer table for $949EA6 + (BTS * 2)
org($949EBE)
	dw(#IcePLM) --0C
	dw(#HyperPLM) --0D
	dw(#SpazerPLM) --0E
	dw(#PlasmaPLM) --0F

-----------------------------------------------------------------
--PLM headers

function makePLM(name)
	dw(##name .. "PLM_Pre", $CBB7)
end

org($84F205)
IcePLM = do
	dw(#IcePLM_Pre, $CBB7)
end
SpazerPLM = makePLM("Spazer")
PlasmaPLM = makePLM("Plasma")
HyperPLM = makePLM("Hyper")

asm

org $84F221
IceBombReveal:
	DW $0001, $F227, $86BC, $0001, $C078, $0000
org $84F22D
HyperBombReveal:
	DW $0001, $F233, $86BC, $0001, $C077, $0000
org $84F239
SpazerBombReveal:
	DW $0001, $F23F, $86BC, $0001, $C079, $0000
org $84F245
PlasmaBombReveal:
	DW $0001, $F24B, $86BC, $0001, $C076, $0000

;---------------------------------------------------------------
;x-ray reaction table mods for $91

org $91D454
	DW $0078
org $91D458
	DW $0077
org $91D45C
	DW $0079
org $91D460
	DW $0076

org $84F105
IcePLM_Pre:
	LDX $0DDE
	LDA $0C18,X
	AND #$0F00
	CMP #$0500
	BEQ ICEAlpha
	LDA $0C18,X
	BIT #$0002
	BNE ICEBeta
	LDA #$0000
	STA $1C37,Y
	RTS
ICEBeta:
	LDX $1C87,Y
	LDA $7F0002,X
	AND #$F000
	ORA #$004D
	STA $1E17,Y
	AND #$8FFF
	STA $7F0002,X
	RTS
ICEAlpha:
	LDA #IceBombReveal
	STA $1D27,Y
	RTS

SpazerPLM_Pre:
	LDX $0DDE
	LDA $0C18,X
	AND #$0F00
	CMP #$0500
	BEQ SPAZERAlpha
	LDA $0C18,X
	BIT #$0004
	BNE SPAZERBeta
	LDA #$0000
	STA $1C37,Y
	RTS
SPAZERBeta:
	LDX $1C87,Y
	LDA $7F0002,X
	AND #$F000
	ORA #$004E
	STA $1E17,Y
	AND #$8FFF
	STA $7F0002,X
	RTS
SPAZERAlpha:
	LDA #SpazerBombReveal
	STA $1D27,Y
	RTS

PlasmaPLM_Pre:
	LDX $0DDE
	LDA $0C18,X
	AND #$0F00
	CMP #$0500
	BEQ PLASMAAlpha
	LDA $0C18,X
	BIT #$0008
	BNE PLASMABeta
	LDA #$0000
	STA $1C37,Y
	RTS
PLASMABeta:
	LDX $1C87,Y
	LDA $7F0002,X
	AND #$F000
	ORA #$004F
	STA $1E17,Y
	AND #$8FFF
	STA $7F0002,X
	RTS
PLASMAAlpha:
	LDA #PlasmaBombReveal
	STA $1D27,Y
	RTS

HyperPLM_Pre:
	LDX $0DDE
	LDA $0C18,X
	AND #$0F00
	CMP #$0500
	BEQ HYPERAlpha
	LDA.w $0C2C,X
	CMP.w #1000
	BPL HYPERBeta
HYPERGamma:
	LDA #$0000
	STA $1C37,Y
	RTS
HYPERBeta:
	LDX $1C87,Y
	LDA $7F0002,X
	AND #$F000
	ORA #$004F
	STA $1E17,Y
	AND #$8FFF
	STA $7F0002,X
	RTS
HYPERAlpha:
	LDA #HyperBombReveal
	STA $1D27,Y
	RTS

end