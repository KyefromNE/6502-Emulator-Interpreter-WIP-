#include <cstdint>
#include <iostream>
#include <bitset>
#include <iomanip>
#include <cstring>


/* NOTE:
	Some code from this repository has been copied directly from a youtube tutorial.

	This was to give me a head start before using this as a project I continued to develop.

	The tutorials and references I am using will be listed here:

	https://www.youtube.com/watch?v=qJgsuQoy9bc&list=PLLwK93hM93Z13TRzPx9JqTIn33feefl37

	https://6502.org/users/obelisk/6502/

	https://csh.rit.edu/~moffitt/docs/6502.html#FLAGS

	https://www.nesdev.org/wiki/

	https://markjames.dev/blog/6502-jump-indirect-bug

	https://6502.org/forum/viewtopic.php?t=1708
*/

/* NOTE:
 *
 * AB/ID functions are WORD functions
 *
 */
using Byte = uint8_t;
using Word = uint16_t;

struct MEM {
	static constexpr uint32_t MAX_MEM = 1024 * 64;

	Byte data[MAX_MEM];

	void init() {

		memset(data, 0, MAX_MEM);
		data[0xFFFC] = 0x00;
		data[0xFFFD] = 0x80;

	}


	Byte operator[](uint32_t address) const {
		return data[address];
	}

	Byte& operator[](uint32_t address) {
		return data[address];
	}
};

struct CPU {



	// Program Counter
	Word PC = 0;

	// Stack Pointer
	Byte SP = 0xFD;

	// Registers

	Byte A = 0;

	Byte X = 0;

	Byte Y = 0;


	// Flags
	Byte C : 1; // Carry Flag
	Byte Z : 1; // Zero Flag
	Byte I : 1; // Interrupt Disable
	Byte D : 1; // Decimal Mode
	Byte V : 1; // Overflow Flag
	Byte N : 1; // Negative Flag



	Byte getstatus(Byte B) {
		return (N << 7) |
		(V << 6) |
		(1 << 5) |   // Always 1 regardless of other status flags
		(B << 4) |   // Invisible Break flag
		(D << 3) |
		(I << 2) |
		(Z << 1) |
		C;
	}


	void reset(MEM& memory) {
		Byte l = memory[0xFFFC];
		Byte h = memory[0xFFFD];
		PC = l | (h << 8);
		SP = 0xFD;
		C = Z = D = V = N = 0;
		A = X = Y = 0;
		I = 1;

	}

	void powerOn(MEM& memory) {
		memory.init();
		reset(memory);
	}

	Byte Fetch(uint32_t& cycles, MEM& memory) {
		// PC is where the next instruction is stored
		Byte Data = memory[PC];
		// PC is incremented, allowing the next use of PC to be the next instruction
		PC++;
		cycles--;
		return Data;
	}

	static Byte read(uint32_t& cycles, uint32_t address, MEM& memory) {
		Byte Data = memory[address];
		cycles--;
		return Data;
	}

	static void write(uint32_t& cycles, uint32_t address, Byte toWrite, MEM& memory) {
		memory[address] = toWrite;
		cycles--;
	}

	void setZN(Byte value) {
		Z = (value == 0);
		N = (value & 0x80) > 0;

	}

	Byte IM(uint32_t& cycles, MEM& memory) {
		Byte value = Fetch(cycles, memory);

		return value;
	}

	Byte ZP(uint32_t& cycles, MEM& memory) {
		Byte zeroPageAddress = Fetch(cycles, memory);

		return zeroPageAddress;
	}

	Byte ZPX(uint32_t& cycles, MEM& memory) {
		Byte zeroPageAddress = Fetch(cycles, memory);
		zeroPageAddress += X;
		cycles--;

		return zeroPageAddress;
	}

	Byte ZPY(uint32_t& cycles, MEM& memory) {
		Byte zeroPageAddress = Fetch(cycles, memory);
		zeroPageAddress += Y;
		cycles--;

		return zeroPageAddress;
	}

	Word AB(uint32_t& cycles, MEM& memory) {
		Byte low = Fetch(cycles, memory);
		Byte high = Fetch(cycles, memory);

		Word combined = low | ((Word)high << 8);

		return combined;
	}

	Word ABXcross(uint32_t& cycles, MEM& memory) {
		Byte low = Fetch(cycles, memory);
		Byte high = Fetch(cycles, memory);

		Word combined = low | ((Word)high << 8);


		Word add = combined + X;

		if ((combined & 0xFF00) != (add & 0xFF00)) {
			cycles--;
		}

		return add;
	}

	Word ABX(uint32_t& cycles, MEM& memory) {
		Byte low = Fetch(cycles, memory);
		Byte high = Fetch(cycles, memory);

		Word combined = low | ((Word)high << 8);


		Word add = combined + X;



		return add;
	}

	Word ABYcross(uint32_t& cycles, MEM& memory) {
		Byte low = Fetch(cycles, memory);
		Byte high = Fetch(cycles, memory);

		Word combined = low | ((Word)high << 8);


		Word add = combined + Y;

		if ((combined & 0xFF00) != (add & 0xFF00)) {
			cycles--;
		}

		return add;
	}

	Word ABY(uint32_t& cycles, MEM& memory) {
		Byte low = Fetch(cycles, memory);
		Byte high = Fetch(cycles, memory);

		Word combined = low | ((Word)high << 8);


		Word add = combined + Y;



		return add;
	}

	Word IDX(uint32_t& cycles, MEM& memory) {
		Byte operand = Fetch(cycles, memory);

		Byte add = operand + X;
		cycles--; //real hardware consumes a cycle here
		Byte highadd = add + 1;
		Byte low = read(cycles, add, memory);
		Byte high = read(cycles, highadd, memory);

		Word combined = low | ((Word)high << 8);


		return combined;

	}

	Word IDYcross(uint32_t& cycles, MEM& memory) {
		Byte operand = Fetch(cycles, memory);
		Byte highop = operand + 1;

		Byte low = read(cycles, operand, memory);
		Byte high = read(cycles, highop, memory);

		Word combined = low | ((Word)high << 8);

		Word add = combined + Y;

		if ((combined & 0xFF00) != (add & 0xFF00)) {
			cycles--;
		}


		return add;
	}

	Word IDY(uint32_t& cycles, MEM& memory) {
		Byte operand = Fetch(cycles, memory);
		Byte highop = operand + 1;

		Byte low = read(cycles, operand, memory);
		Byte high = read(cycles, highop, memory);

		Word combined = low | ((Word)high << 8);

		Word add = combined + Y;


		return add;
	}

	static void pushToStack(uint32_t& cycles, Byte& SP, Byte whatToStack, MEM& memory) {
		//SP lives 0x0100-0x01FF, since SP is initialised as 0xFD, 0x100 + 0xFD ensures it's within range of where the stack lives
		memory[0x0100 + SP] = whatToStack;
		cycles--;
		SP--;
		cycles--;
	}

	static Byte pullFromStack(uint32_t& cycles, Byte& SP, MEM& memory) {
		SP++;
		cycles--;
		Byte op = memory[0x0100 + SP];
		return op;

	}



	static constexpr Byte
		INS_LDA_IM = 0xA9,
		INS_LDA_ZP = 0xA5,
		INS_LDA_ZPX = 0xB5,
		INS_LDA_AB = 0xAD,
		INS_LDA_ABX = 0xBD,
		INS_LDA_ABY = 0xB9,
		INS_LDA_IDX = 0xA1,
		INS_LDA_IDY = 0xB1,
		INS_LDX_IM = 0xA2,
		INS_LDX_ZP = 0xA6,
		INS_LDX_ZPY = 0xB6,
		INS_LDX_AB = 0xAE,
		INS_LDX_ABY = 0xBE,
		INS_LDY_IM = 0xA0,
		INS_LDY_ZP = 0xA4,
		INS_LDY_ZPX = 0xB4,
		INS_LDY_AB = 0xAC,
		INS_LDY_ABX = 0xBC,
		INS_STA_ZP = 0x85,
		INS_STA_ZPX = 0x95,
		INS_STA_AB = 0x8D,
		INS_STA_ABX = 0x9D,
		INS_STA_ABY = 0x99,
		INS_STA_IDX = 0x81,
		INS_STA_IDY = 0x91,
		INS_STX_ZP = 0x86,
		INS_STX_ZPY = 0x96,
		INS_STX_AB = 0x8E,
		INS_STY_ZP = 0x84,
		INS_STY_ZPX = 0x94,
		INS_STY_AB = 0x8C,
		INS_TAX_I = 0xAA,
		INS_TAY_I = 0xA8,
		INS_TSX_I = 0xBA,
		INS_TXA_I = 0x8A,
		INS_TXS_I = 0x9A,
		INS_TYA_I = 0x98,
		INS_PHA_I = 0x48,
		INS_PHP_I = 0x08,
		INS_PLA_I = 0x68,
		INS_PLP_I = 0x28,
		INS_NOP_I = 0xEA,
		INS_AND_IM = 0x29,
		INS_AND_ZP = 0x25,
		INS_AND_ZPX = 0x35,
		INS_AND_AB = 0x2D,
		INS_AND_ABX = 0x3D,
		INS_AND_ABY = 0x39,
		INS_AND_IDX = 0x21,
		INS_AND_IDY = 0x31,
		INS_ORA_IM = 0x09,
		INS_ORA_ZP = 0x05,
		INS_ORA_ZPX = 0x15,
		INS_ORA_AB = 0x0D,
		INS_ORA_ABX = 0x1D,
		INS_ORA_ABY = 0x19,
		INS_ORA_IDX = 0x01,
		INS_ORA_IDY = 0x11,
		INS_EOR_IM = 0x49,
		INS_EOR_ZP = 0x45,
		INS_EOR_ZPX = 0x55,
		INS_EOR_AB = 0x4D,
		INS_EOR_ABX = 0x5D,
		INS_EOR_ABY = 0x59,
		INS_EOR_IDX = 0x41,
		INS_EOR_IDY = 0x51,
		INS_BIT_ZP = 0x24,
		INS_BIT_AB = 0x2C,
		INS_JMP_AB = 0x4C,
		INS_JMP_IND = 0x6C,			// How unique
		INS_ASL_A = 0x0A,
		INS_ASL_ZP = 0x06,
		INS_ASL_ZPX = 0x16,
		INS_ASL_AB = 0x0E,
		INS_ASL_ABX = 0x1E,
		INS_LSR_A = 0x4A,
		INS_LSR_ZP = 0x46,
		INS_LSR_ZPX = 0x56,
		INS_LSR_AB = 0x4E,
		INS_LSR_ABX = 0x5E,
		INS_ROL_A = 0x2A,
		INS_ROL_ZP = 0x26,
		INS_ROL_ZPX = 0x36,
		INS_ROL_AB = 0x2E,
		INS_ROL_ABX = 0x3E,
		INS_ROR_A = 0x2A,
		INS_ROR_ZP = 0x26,
		INS_ROR_ZPX = 0x36,
		INS_ROR_AB = 0x2E,
		INS_ROR_ABX = 0x3E,
		INS_JSR_AB = 0x20,
		INS_RTS_I = 0x60,
		INS_BRK_I = 0x00,
		INS_RTI_I = 0x40;

	void execute(uint32_t cycles, MEM& memory) {
		while (cycles > 0) {
			Byte ins = Fetch(cycles, memory);

			switch (ins) {
				case INS_LDA_IM: {
					A = IM(cycles, memory);
					setZN(A);
				} break;

				case INS_LDA_ZP: {
					A = read(cycles, ZP(cycles, memory), memory);
					setZN(A);
				} break;

				case INS_LDA_ZPX: {
					A = read(cycles, ZPX(cycles, memory), memory);
					setZN(A);
				} break;
				case INS_LDA_AB: {
					A = read(cycles, AB(cycles, memory), memory);
					setZN(A);
				} break;
				case INS_LDA_ABX: {
					A = read(cycles, ABXcross(cycles, memory), memory);
					setZN(A);
				} break;
				case INS_LDA_ABY: {
					A = read(cycles, ABYcross(cycles, memory), memory);
					setZN(A);
				} break;
				case INS_LDA_IDX: {

					A = read(cycles, IDX(cycles, memory), memory);
					setZN(A);
				} break;
				case INS_LDA_IDY: {
					A = read(cycles, IDYcross(cycles, memory), memory);
					setZN(A);
				} break;
				case INS_LDX_IM: {
					X = IM(cycles, memory);
					setZN(X);
				} break;
				case INS_LDX_ZP: {
					X = read(cycles, ZP(cycles, memory), memory);
					setZN(X);
				} break;
				case INS_LDX_ZPY: {
					X = read(cycles, ZPY(cycles, memory), memory);
					setZN(X);
				} break;
				case INS_LDX_AB: {
					X = read(cycles, AB(cycles, memory), memory);
					setZN(X);
				} break;
				case INS_LDX_ABY: {
					X = read(cycles, ABYcross(cycles, memory), memory);
					setZN(X);
				} break;
				case INS_LDY_IM: {
					Y = IM(cycles, memory);
					setZN(Y);
				} break;
				case INS_LDY_ZP: {
					Y = read(cycles, ZP(cycles, memory), memory);
					setZN(Y);
				} break;
				case INS_LDY_ZPX: {
					Y = read(cycles, ZPX(cycles, memory), memory);
					setZN(Y);
				} break;
				case INS_LDY_AB: {
					Y = read(cycles, AB(cycles, memory), memory);
					setZN(Y);
				} break;
				case INS_LDY_ABX: {
					Y = read(cycles, ABXcross(cycles, memory), memory);
					setZN(Y);
				} break;
				case INS_STA_ZP: {
					write(cycles, ZP(cycles, memory), A, memory);
				} break;
				case INS_STA_ZPX: {
					write(cycles, ZPX(cycles, memory), A, memory);
				} break;
				case INS_STA_AB: {
					write(cycles, AB(cycles, memory), A, memory);

				} break;
				case INS_STA_ABX: {
					write(cycles, ABX(cycles, memory), A, memory);
					cycles--;
				} break;
				case INS_STA_ABY: {
					write(cycles, ABY(cycles, memory), A, memory);
					cycles--;
				} break;
				case INS_STA_IDX: {
					write(cycles, IDX(cycles, memory), A, memory);
				} break;
				case INS_STA_IDY: {
					write(cycles, IDY(cycles, memory), A, memory);
					cycles--;
				} break;
				case INS_STX_ZP: {
					write(cycles, ZP(cycles, memory), X, memory);
				} break;
				case INS_STX_ZPY: {
					write(cycles, ZPY(cycles, memory), X, memory);
				} break;
				case INS_STX_AB: {
					write(cycles, AB(cycles, memory), X, memory);
				} break;
				case INS_STY_ZP: {
					write(cycles, ZP(cycles, memory), Y, memory);
				} break;
				case INS_STY_ZPX: {
					write(cycles, ZPX(cycles, memory), Y, memory);
				} break;
				case INS_STY_AB: {
					write(cycles, AB(cycles, memory), Y, memory);
				} break;
				case INS_TAX_I: {
					X = A;
					cycles--;
					setZN(X);
				} break;
				case INS_TAY_I: {
					Y = A;
					cycles--;
					setZN(Y);
				} break;
				case INS_TSX_I: {
					X = SP;
					cycles--;
					setZN(X);
				} break;
				case INS_TXA_I: {
					A = X;
					cycles--;
					setZN(A);
				} break;
				case INS_TXS_I: {
					SP = X;
					cycles--;
				} break;
				case INS_TYA_I: {
					A = Y;
					cycles--;
					setZN(A);
				} break;
				case INS_PHA_I: {
					pushToStack(cycles, SP, A, memory);
					cycles--;
				} break;
				case INS_PHP_I: {
					pushToStack(cycles, SP, getstatus(0), memory);
					cycles--;
				} break;
				case INS_PLA_I: {
					A = pullFromStack(cycles, SP, memory);
					cycles--;
					setZN(A);
					cycles--;
				} break;
				case INS_PLP_I: {
					Byte PLP = pullFromStack(cycles, SP, memory);
					cycles--;
					std::bitset<8> bits(PLP);


					C = bits[0];
					Z = bits[1];
					I = bits[2];
					D = bits[3];
					V = bits[6];
					N = bits[7];
					cycles--;


				} break;
				case INS_NOP_I: {
					// No operation
					cycles--;
				} break;
				case INS_AND_IM: {
					Byte comp = IM(cycles, memory);

					A = comp & A;

					setZN(A);
				} break;
				case INS_AND_ZP: {
					Byte comp = read(cycles, ZP(cycles, memory), memory);


					A = comp & A;

					setZN(A);
				} break;
				case INS_AND_ZPX: {
					Byte comp = read(cycles, ZPX(cycles, memory), memory);

					A = comp & A;

					setZN(A);

				} break;
				case INS_AND_AB: {
					Byte comp = read(cycles, AB(cycles, memory), memory);

					A = comp & A;

					setZN(A);
				} break;
				case INS_AND_ABX: {
					Byte comp = read(cycles, ABXcross(cycles, memory), memory);

					A = comp & A;

					setZN(A);
				} break;
				case INS_AND_ABY: {
					Byte comp = read(cycles, ABYcross(cycles, memory), memory);

					A = comp & A;

					setZN(A);
				} break;
				case INS_AND_IDX: {
					Byte comp = read(cycles, IDX(cycles, memory), memory);

					A = comp & A;

					setZN(A);
				} break;
				case INS_AND_IDY: {
					Byte comp = read(cycles, IDYcross(cycles, memory), memory);

					A = comp & A;

					setZN(A);
				} break;
				case INS_ORA_IM: {
					Byte comp = IM(cycles, memory);

					A = A | comp;

					setZN(A);
				} break;
				case INS_ORA_ZP: {
					Byte comp = read(cycles, ZP(cycles, memory), memory);

					A = A | comp;

					setZN(A);
				} break;
				case INS_ORA_ZPX: {
					Byte comp = read(cycles, ZPX(cycles, memory), memory);

					A = A | comp;

					setZN(A);
				} break;
				case INS_ORA_AB: {
					Byte comp = read(cycles, AB(cycles, memory), memory);

					A = A | comp;

					setZN(A);
				} break;
				case INS_ORA_ABX: {
					Byte comp = read(cycles, ABXcross(cycles, memory), memory);

					A = A | comp;

					setZN(A);
				} break;
				case INS_ORA_ABY: {
					Byte comp = read(cycles, ABYcross(cycles, memory), memory);

					A = A | comp;

					setZN(A);
				} break;
				case INS_ORA_IDX: {
					Byte comp = read(cycles, IDX(cycles, memory), memory);

					A = A | comp;

					setZN(A);
				} break;
				case INS_ORA_IDY: {
					Byte comp = read(cycles, IDYcross(cycles, memory), memory);

					A = A | comp;

					setZN(A);
				} break;
				case INS_EOR_IM: {
					A = A ^ IM(cycles, memory);

					setZN(A);
				} break;
				case INS_EOR_ZP: {
					A = A ^ read(cycles, ZP(cycles, memory), memory);

					setZN(A);
				} break;
				case INS_EOR_ZPX: {
					A = A ^ read(cycles, ZPX(cycles, memory), memory);

					setZN(A);
				} break;
				case INS_EOR_AB: {
					A = A ^ read(cycles, AB(cycles, memory), memory);

					setZN(A);
				} break;
				case INS_EOR_ABX: {
					A = A ^ read(cycles, ABXcross(cycles, memory), memory);

					setZN(A);
				} break;
				case INS_EOR_ABY: {
					A = A ^ read(cycles, ABYcross(cycles, memory), memory);

					setZN(A);
				} break;
				case INS_EOR_IDX: {
					A = A ^ read(cycles, IDX(cycles, memory), memory);

					setZN(A);
				} break;
				case INS_EOR_IDY: {
					A = A ^ read(cycles, IDYcross(cycles, memory), memory);

					setZN(A);
				} break;
				case INS_BIT_ZP: {
					Byte result = read(cycles, ZP(cycles, memory), memory);

					std::bitset<8> bits(result);

					N = bits[7];
					V = bits[6];
					Z = ((A & result) == 0);
				} break;
				case INS_BIT_AB: {
					Byte result = read(cycles, AB(cycles, memory), memory);

					std::bitset<8> bits(result);

					N = bits[7];
					V = bits[6];
					Z = ((A & result) == 0);
				} break;
				case INS_JMP_AB: {

					Word set = AB(cycles, memory);


					PC = set;


				} break;
				case INS_JMP_IND: {

					Byte low = Fetch(cycles, memory);
					Byte high = Fetch(cycles, memory);

					Word add = ((Word)high << 8) | low;

					Byte PClow = read(cycles, add, memory);

					Byte PChigh;

					if ((add & 0xFF) == 0xFF) {
						PChigh = read(cycles, (add & 0xFF00), memory);
					}
					else {
						PChigh = read(cycles, add + 1, memory);
					}

					PC = ((Word)PChigh << 8) | PClow;

				} break;
				case INS_ASL_A: {
					std::bitset<8> bits(A);

					C = bits[7];

					A = A << 1;

					cycles--;

					std::bitset<8> resbits(A);

					N = resbits[7];
					Z = (A == 0);

				} break;
				case INS_ASL_ZP: {

					Byte address = ZP(cycles, memory);

					Byte temp = read(cycles, address, memory);

					std::bitset<8> bits(temp);

					C = bits[7];

					Byte shift = temp << 1;

					write(cycles, address, shift, memory);

					std::bitset<8> resbits(shift);

					N = resbits[7];
					Z = (shift == 0);

					cycles--;


				} break;
				case INS_ASL_ZPX: {

					Byte address = ZPX(cycles, memory);

					Byte temp = read(cycles, address, memory);

					std::bitset<8> bits(temp);

					C = bits[7];

					Byte shift = temp << 1;

					write(cycles, address, shift, memory);

					std::bitset<8> resbits(shift);

					N = resbits[7];
					Z = (shift == 0);

					cycles--;
				} break;
				case INS_ASL_AB: {
					Word address = AB(cycles, memory);

					Byte temp = read(cycles, address, memory);

					std::bitset<8> bits(temp);

					C = bits[7];

					Byte shift = temp << 1;

					write(cycles, address, shift, memory);

					std::bitset<8> resbits(shift);

					N = resbits[7];
					Z = (shift == 0);

					cycles--;
				} break;
				case INS_ASL_ABX: {

					Word address = ABX(cycles, memory);

					Byte temp = read(cycles, address, memory);

					std::bitset<8> bits(temp);

					C = bits[7];
					cycles--;
					
					Byte shift = temp << 1;

					cycles--;

					write(cycles, address, shift, memory);

					std::bitset<8> resbits(shift);

					N = resbits[7];
					Z = (shift == 0);
				} break;
				case INS_LSR_A: {
					std::bitset<8> bits(A);

					C = bits[0];

					A = A >> 1;

					cycles--;

					N = 0;
					Z = (A == 0);
				} break;
				case INS_LSR_ZP: {

					Word address = ZP(cycles, memory);

					Byte temp = read(cycles, address, memory);

					std::bitset<8> bits(temp);

					C = bits[0];

					Byte shift = temp >> 1;

					cycles--;

					write(cycles, address, shift, memory);

					N = 0;
					Z = (shift == 0);
				} break;
				case INS_LSR_ZPX: {
					Word address = ZPX(cycles, memory);

					Byte temp = read(cycles, address, memory);

					std::bitset<8> bits(temp);

					C = bits[0];

					Byte shift = temp >> 1;

					cycles--;

					write(cycles, address, shift, memory);

					N = 0;
					Z = (shift == 0);
				} break;
				case INS_LSR_AB: {
					Word address = AB(cycles, memory);

					Byte temp = read(cycles, address, memory);

					std::bitset<8> bits(temp);

					C = bits[0];

					Byte shift = temp >> 1;

					cycles--;

					write(cycles, address, shift, memory);


					N = 0;
					Z = (shift == 0);
				} break;
				case INS_LSR_ABX: {

					Word address = ABX(cycles, memory);

					Byte temp = read(cycles, address, memory);

					std::bitset<8> bits(temp);

					C = bits[0];

					Byte shift = temp >> 1;

					cycles--;

					write(cycles, address, shift, memory);


					N = 0;
					Z = (shift == 0);

					cycles--;

				} break;
				case INS_ROL_A: {
					std::bitset<8> bits(A);

					Byte temp = C;

					C = bits[7];

					A = A << 1;

					cycles--;

					A = A | temp;

					std::bitset<8> resbits(A);

					N = resbits[7];
					Z = (A == 0);

				} break;
				case INS_ROL_ZP: {

					Byte address = ZP(cycles, memory);

					Byte temp = C;

					Byte operand = read(cycles, address, memory);

					std::bitset<8> bits(operand);

					C = bits[7];

					operand = operand << 1;

					cycles--;

					operand = operand | temp;

					write(cycles, address, operand, memory);

					std::bitset<8> resbits(operand);

					N = resbits[7];
					Z = (operand == 0);
				} break;
				case INS_ROL_ZPX: {

					Byte address = ZPX(cycles, memory);

					Byte temp = C;

					Byte operand = read(cycles, address, memory);

					std::bitset<8> bits(operand);

					C = bits[7];

					operand = operand << 1;

					cycles--;

					operand = operand | temp;

					write(cycles, address, operand, memory);

					std::bitset<8> resbits(operand);

					N = resbits[7];
					Z = (operand == 0);
				} break;
				case INS_ROL_AB: {
					Word address = AB(cycles, memory);

					Byte temp = C;

					Byte operand = read(cycles, address, memory);

					std::bitset<8> bits(operand);

					C = bits[7];

					operand = operand << 1;

					cycles--;

					operand = operand | temp;

					write(cycles, address, operand, memory);

					std::bitset<8> resbits(operand);

					N = resbits[7];
					Z = (operand == 0);
				} break;
				case INS_ROL_ABX: {

					Byte address = ABX(cycles, memory);

					Byte temp = C;

					Byte operand = read(cycles, address, memory);

					std::bitset<8> bits(operand);

					C = bits[7];

					operand = operand << 1;

					cycles--;

					operand = operand | temp;

					write(cycles, address, operand, memory);

					std::bitset<8> resbits(operand);

					N = resbits[7];
					Z = (operand == 0);
				} break;
				case INS_ROR_A: {
					Byte temp = C;
					
					std::bitset<8> bits(A);
					
					C = bits[0];
					
					A = A >> 1;
					
					A = A | temp;
					
					std::bitset<8> resbits(A);
					
					N = resbits[7];
					Z = (A == 0)
					
				} break;
				case INS_ROR_ZP: {

					Byte temp = C;
					
					Byte address = ZP(cycles, memory);
					
					Byte operand = read(cycles, address, memory);
					
					std::bitset<8> bits(operand);
					
					C = bits[0];
					
					operand = operand >> 1;
					
					cycles--;
					 
					operand = operand | temp;
					
					write(cycles, address, operand, memory);
					
					std::bitset<8> resbits(operand);
					
					Z = (operand == 0);
					N = resbits[7];
				} break;
				case INS_ROR_ZPX: {
					
					Byte temp = C;
					
					Byte address = ZPX(cycles, memory);
					
					Byte operand = read(cycles, address, memory);
					
					std::bitset<8> bits(operand);
					
					C = bits[0];
					
					operand = operand >> 1;
					
					cycles--;
					 
					operand = operand | temp;
					
					write(cycles, address, operand, memory);
					
					std::bitset<8> resbits(operand);
					
					Z = (operand == 0);
					N = resbits[7];

				} break;
				case INS_ROR_AB: {
					Byte temp = C;
					
					Word address = AB(cycles, memory);
					
					Byte operand = read(cycles, address, memory);
					
					std::bitset<8> bits(operand);
					
					C = bits[0];
					
					operand = operand >> 1;
					
					cycles--;
					 
					operand = operand | temp;
					
					write(cycles, address, operand, memory);
					
					std::bitset<8> resbits(operand);
					
					Z = (operand == 0);
					N = resbits[7];
				} break;
				case INS_ROR_ABX: {
					
					Byte temp = C;
					
					Word address = ABX(cycles, memory);
					
					Byte operand = read(cycles, address, memory);
					
					std::bitset<8> bits(operand);
					
					C = bits[0];
					
					operand = operand >> 1;
					
					cycles--;
					 
					operand = operand | temp;
					
					write(cycles, address, operand, memory);
					
					std::bitset<8> resbits(operand);
					
					Z = (operand == 0);
					N = resbits[7];
				} break;
				case INS_JSR_AB: {
					Byte low = Fetch(cycles, memory);
					
					Byte high = Fetch(cycles, memory);
					
					// Address to return to
					Word address = PC - 1;
					
					pushToStack(cycles, SP, (address >> 8) & 0xFF, memory);
					pushToStack(cycles, SP, (address & 0xFF), memory);
					
					PC = low | ((Word)high << 8);
				
				} break;
				case INS_RTS_I: {
					Byte low = pullFromStack(cycles, SP, memory);
					cycles--;
					Byte high = pullFromStack(cycles, SP, memory);
					cycles--;
					PC = low | ((Word)high << 8);
					cycles--;
					PC++;
					cycles--;
				} break;
				case INS_BRK_I: {
					pushToStack(cycles, SP, (PC >> 8) & 0xFF, memory);
					pushToStack(cycles, SP, PC & 0xFF, memory);
					pushToStack(cycles, SP, getstatus(1), memory);
					
					
					I = 1;
					cycles--;
				} break;
				case INS_RTI_I: {
					Byte status = pullFromStack(cycles, SP, memory);
					
					Byte low = pullFromStack(cycles, SP, memory);
					
					Byte high = pullFromStack(cycles, SP, memory);
					
					std::bitset<8> bits(status);
					
					C = bits[0];
					Z = bits[1];
					I = bits[2];
					D = bits[3];
					V = bits[6];
					N = bits[7];
					
					PC = low | ((Word)high << 8);
				} break;
				default: {
					std::cout << "Instruction Not Handled!!! OH GOD!!!!! KJJHKHJHJHJGHGHKGJHKHGJK!!!!!!";
					cycles = 0;
				} break;

			}
		}


	}

};

// "Static" key word does not allow it to store a massive amount of data on the stack (C6262 error)
static MEM mem;
CPU cpu;

int main() {

	cpu.powerOn(mem);

	cpu.X = 0x01;

	mem[0x8000] = CPU::INS_LDY_ABX;
	mem[0x8001] = 0xAB;
	mem[0x8002] = 0xCD;

	mem[0xCDAC] = 0x84;

	cpu.execute(4, mem);

	std::cout
		<< std::hex
		<< "Y:  " << static_cast<int>(cpu.Y) << '\n'
		<< "X:  " << static_cast<int>(cpu.X) << '\n'
		<< "PC: " << static_cast<int>(cpu.PC) << '\n'
		<< "Z:  " << static_cast<int>(cpu.Z) << '\n'
		<< "N:  " << static_cast<int>(cpu.N) << '\n';


	return 0;
}
