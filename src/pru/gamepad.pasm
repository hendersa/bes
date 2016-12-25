// PRU firmware to drive the Beagle Entertainment System 
// SNES gamepad cape. Four GPIOs are used by the firmware:
//
// OUTPUTS
// Clock: P9.11, GPIO0[30]
// Latch: P9.13, GPIO0[31]
// INPUTS
// Gamepad0: P9.12, GPIO1[28]
// Cape:    Gamepad1: P8.26, GPIO1[29]
// Console: Gamepad1: P9.14, GPIO1[18]

.origin 0
.entrypoint INIT

// Useful constants
#define GPIO0_DATA_IN_ADDR	#0x44E07138
#define GPIO0_DATA_OUT_ADDR	#0x44E0713C
#define GPIO1_DATA_IN_ADDR	#0x4804C138
#define GPIO1_DATA_OUT_ADDR	#0x4804C13C
#define GPIO2_DATA_IN_ADDR	#0x481AC138
#define GPIO2_DATA_OUT_ADDR	#0x481AC13C
#define GPIO3_DATA_IN_ADDR	#0x481AE138
#define GPIO3_DATA_OUT_ADDR	#0x481AE13C

// Offsets of important bits in the GPIO registers
#define GAMEPAD0_BIT		#28
#define GAMEPAD1_BIT		#18
#define CLOCK_BIT		t30
#define LATCH_BIT		t31

// Defines for regs to make it easier to understand 
// ops using register-based constants or variables.
#define GPIO0_OUT_REG		r4
#define GPIO1_IN_REG		r5
#define BUTTON_COUNT_REG	r6
#define GAMEPAD_STATE_REG	r25
#define DELAY_COUNT_REG		r29
#define GAMEPAD0_REG		r12
#define GAMEPAD1_REG		r13

INIT:
	// Enable OCP master port
	LBCO	r0, C4, #4, #4
	CLR	r0.t4
	SBCO	r0, C4, #4, #4

	// Configure the programmable pointer register for PRU0 
	// by setting c28_pointer[15:0] field to 0x0120.  This 
	// will make C28 point to 0x00012000 (PRU shared RAM).
	MOV	r0, #0x00000120
	MOV	r1, #0x22028	// CTPPR_0
	SBBO	r0, r1, #0, #4

	// Configure the programmable pointer register for 
	// PRU0 by setting c31_pointer[15:0] field to 0x0010.
	// This will make C31 point to 0x80001000 (DDR memory).
	MOV	r0, #0x00100000
	MOV	r1, #0x2202C	// CTPPR_1
	SBBO	r0, r1, #0, #4

	// Set up the constant registers
	MOV	GPIO0_OUT_REG, GPIO0_DATA_OUT_ADDR
	MOV	GPIO1_IN_REG, GPIO1_DATA_IN_ADDR

BEGIN_POLL:
	// Set latch
	LBBO	r0, GPIO0_OUT_REG, #0, #4
	SET	r0.LATCH_BIT
	SBBO	r0, GPIO0_OUT_REG, #0, #4

	CALL	DELAY_6US
	CALL	DELAY_6US

	// Clear latch and set clock
	LBBO	r0, GPIO0_OUT_REG, #0, #4
	CLR	r0.LATCH_BIT
	SET	r0.CLOCK_BIT
	SBBO	r0, GPIO0_OUT_REG, #0, #4

	// Setup button state loop
	MOV	BUTTON_COUNT_REG, #16
	MOV	GAMEPAD_STATE_REG, #0	

BUTTON_LOOP:
	CALL	DELAY_6US

	// Clear clock
	LBBO	r0, GPIO0_OUT_REG, #0, #4
	CLR	r0.CLOCK_BIT
	SBBO	r0, GPIO0_OUT_REG, #0, #4

	// Read and isolate current gamepad data
	LBBO	r0, GPIO1_IN_REG, #0, #4
	LSR	GAMEPAD0_REG, r0, GAMEPAD0_BIT
	AND	GAMEPAD0_REG, GAMEPAD0_REG, #1
	LSR	GAMEPAD1_REG, r0, GAMEPAD1_BIT
	AND	GAMEPAD1_REG, GAMEPAD1_REG, #1

	// Shift gamepad state and store new bit
	LSL	GAMEPAD_STATE_REG, GAMEPAD_STATE_REG, #1
	OR	GAMEPAD_STATE_REG, GAMEPAD_STATE_REG, GAMEPAD0_REG 
	LSL	GAMEPAD_STATE_REG, GAMEPAD_STATE_REG, #1
	OR	GAMEPAD_STATE_REG, GAMEPAD_STATE_REG, GAMEPAD1_REG

	CALL	DELAY_6US

	// Set clock
	LBBO	r0, GPIO0_OUT_REG, #0, #4
	SET	r0.CLOCK_BIT
	SBBO	r0, GPIO0_OUT_REG, #0, #4

	// Decrement counter and loop if needed
	SUB	BUTTON_COUNT_REG, BUTTON_COUNT_REG, #1
	QBNE	BUTTON_LOOP, BUTTON_COUNT_REG, #0
BUTTON_CLEANUP:
	// Store gamepad bits into PRU shared memory
	MOV	r0, GAMEPAD_STATE_REG
	SBCO	r0, C28, #0, #4      // CONST_PRUSHAREDRAM

	// Delay 16.4 ms and then loop around again
	CALL 	DELAY_16MS
	QBA	BEGIN_POLL

// 2000 per 1 us
// 6us delay subroutine
DELAY_6US:
	// 6us * (2000/us) = 12,000
	MOV	DELAY_COUNT_REG, #12000
DELAY_6US_LOOP:
	SUB	DELAY_COUNT_REG, DELAY_COUNT_REG, #1
	QBNE	DELAY_6US_LOOP, DELAY_COUNT_REG, #0
	RET

// 16.4ms delay subroutine
// Actually, do far less delay for better responsiveness
DELAY_16MS:
	// 16.4us * 1000 * (2000/us) = 32,800,000
	MOV	DELAY_COUNT_REG, #4100000
DELAY_16MS_LOOP:
	SUB	DELAY_COUNT_REG, DELAY_COUNT_REG, #1
	QBNE	DELAY_16MS_LOOP, DELAY_COUNT_REG, #0
	RET

// Shutdown (which we never reach)
MOV	r31.b0, 19+16 // PRU0_ARM_INTERRUPT == 19

HALT
	
