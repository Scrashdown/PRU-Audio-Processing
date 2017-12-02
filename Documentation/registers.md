## Registers for one channel :

	INT0 R0
	INT1 R1
	INT2 R2
	INT3 R3

	COMB0 R5
	COMB1 R6
	COMB2 R7
	OUTPUT R8

	LAST_INT R4
	LAST_COMB0 R9
	LAST_COMB1 R10
	LAST_COMB2 R11

	TMP_REG R29
	SAMPLE_COUNTER R28
	LOCAL_MEM R27


## Registers for 2 channels :

	INT0 R0-R1
	INT1 R2-R3
	INT2 R4-R5
	INT3 R6-R7
	LAST_INT R8-R9

	COMB0 R10-R11
	COMB1 R12-R13
	COMB2 R14-R15

	LAST_COMB0 R16-R17
	LAST_COMB1 R18-R19
	LAST_COMB2 R20-R21

	total: 22 registers

	OUTPUT R16-R17  (shared)
	TMP_REG R29
	SAMPLE_COUNTER R28
	LOCAL_MEM R27


## Registers for 8 channels :

XCHG doesn't work: /!\ need to use the XIN/XOUT shift functionality which uses r0.b0 to store the shift

	CHAN12 -> BANK0 r1-r22
	CHAN34 -> BANK1 r1-r22
	CHAN56 -> BANK2 r1-r22
	CHAN78 -> BANK0 r23-r29 + BANK1 r23-r29 + BANK2 r23-r29
	
	OUTPUT -> PRU1 r23-r26 (4 registers => can write 4 channels at a time!)
	TMP_REG -> PRU1 r27
	SAMPLE_COUNTER -> PRU1 r28
	LOCAL_MEM -> PRU1 r29


## Registers for 6 channels :

XCHG doesn't work: /!\ need to use the XIN/XOUT instructions shift functionality which uses r0.b0 to store the shift

	CHAN123 -> BANK0 r1-r22 (ch.1-2) + BANK1 r1-r11 (ch.3)
	CHAN456 -> BANK1 r12-r22 (ch.4) + BANK2 r1-r22 (ch.5-6)

	BANK1_SHIFT -> r0.b0
	OUTPUT -> PRU1 r23-r25 (3 registers => can write 3 channels (/6) at a time!)
	TMP_REG -> PRU1 r27
	SAMPLE_COUNTER -> PRU1 r28
	LOCAL_MEM -> PRU1 r29