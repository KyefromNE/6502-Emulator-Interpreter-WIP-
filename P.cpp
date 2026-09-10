#include <cstdint>
#include <iostream>

using Byte = uint8_t;
using Word = uint16_t;

struct MEM {
    static constexpr uint32_t MAX_MEM = 1024 * 64;

    Byte data[MAX_MEM];

    void init() {
        
		data[MAX_MEM] = {0};
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
    Word PC;

    // Stack Pointer
    Byte SP;

    // Registers

    Byte A, X, Y;


    // Flags
    Byte C : 1; // Carry Flag
    Byte Z : 1; // Zero Flag
    Byte I : 1; // Interrupt Disable
    Byte D : 1; // Decimal Mode
    Byte B : 1; // Break Command, generate for PHP/BRK/interrupts push the status
    Byte V : 1; // Overflow Flag
    Byte N : 1; // Negative Flag


	Byte status =
		(N << 7) |
		(V << 6) |
		(1 << 5) |   // has no purpose
		(1 << 4) |   // B flag is not used here on native hardware
		(D << 3) |
		(I << 2) |
		(Z << 1) |
		C;
    void reset(MEM& memory) {
        memory.init();
        Byte l = memory[0xFFFC];
        Byte h = memory[0xFFFD];
        PC = l | (h << 8);
        SP = 0xFD;
        C = Z = I = D = B = V = N = 0;
        A = X = Y = 0;

    }
    Byte Fetch(uint32_t& cycles, MEM& memory) {
        Byte Data = memory[PC];
        PC++;
        cycles--;
        return Data;
    }

    Byte read(uint32_t& cycles, uint32_t address, MEM& memory) {
        Byte Data = memory[address];
        cycles--;
        return Data;
    }
    
    void write(uint32_t& cycles, uint32_t address, Byte toWrite, MEM& memory) {
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
					
		Word combined = low | (high << 8);
		
		return combined;
	}
	
	Word ABXcross(uint32_t& cycles, MEM& memory) {
		Byte low = Fetch(cycles, memory);
		Byte high = Fetch(cycles, memory);
					
		Word combined = low | (high << 8);
					
					
		Word add = combined + X;
					
		if ((combined & 0xFF00) != (add & 0xFF00)) {
			cycles--;
		}
		
		return add;
	}
	
	Word ABX(uint32_t& cycles, MEM& memory) {
		Byte low = Fetch(cycles, memory);
		Byte high = Fetch(cycles, memory);
					
		Word combined = low | (high << 8);
					
					
		Word add = combined + X;
					
		
		
		return add;
	}
	
	Word ABYcross(uint32_t& cycles, MEM& memory) {
		Byte low = Fetch(cycles, memory);
		Byte high = Fetch(cycles, memory);
					
		Word combined = low | (high << 8);
					
					
		Word add = combined + Y;
					
		if ((combined & 0xFF00) != (add & 0xFF00)) {
			cycles--;
		}
		
		return add;
	}
	
	Word ABY(uint32_t& cycles, MEM& memory) {
		Byte low = Fetch(cycles, memory);
		Byte high = Fetch(cycles, memory);
					
		Word combined = low | (high << 8);
					
					
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
					
		Word combined = low | (high << 8);
		
		
		return combined;
					
	}
	
	Word IDYcross(uint32_t& cycles, MEM& memory) {
		Byte operand = Fetch(cycles, memory);
		Byte highop = operand + 1; 
				
		Byte low = read(cycles, operand, memory);
		Byte high = read(cycles, highop, memory);
					
		Word combined = low | (high << 8);
					
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
					
		Word combined = low | (high << 8);
					
		Word add = combined + Y;
		
		
		return add;
	}
	
	void pushToStack(uint32_t& cycles, Byte& SP, Byte whatToStack, MEM& memory) {
		//SP lives 0x0100-0x01FF
		memory[0x0100 + SP] = whatToStack;
		cycles--;
		SP--;
		cycles--;
	}

	Byte pullFromStack(uint32_t& cycles, Byte& SP, MEM& memory) {
		SP++;
		
		Byte op = memory[0x0100 + SP];

		memory[0x0100 + SP] = 0;

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
		INS_PLP_I = 0x28;


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
				} break;
				case INS_STA_ABY: {
					write(cycles, ABY(cycles, memory), A, memory);
				} break;
				case INS_STA_IDX: {
					write(cycles, IDX(cycles, memory), A, memory);
				} break;
				case INS_STA_IDY: {
					write(cycles, IDY(cycles, memory), A, memory);
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
					setZN(X);
				} break;
				case INS_TAY_I: {
					Y = A;
					setZN(Y);
				} break;
				case INS_TSX_I: {
					X = SP;
					setZN(X);
				} break;
				case INS_TXA_I: {
					A = X;
					setZN(A);
				} break;
				case INS_TXS_I: {
					SP = X;
				} break;
				case INS_TYA_I: {
					A = Y;
					setZN(A);
				} break;
				case INS_PHA_I: {
					pushToStack(cycles, SP, A, memory);
				} break;
				case INS_PHP_I: {
					pushToStack(cycles, SP, status, memory);
				} break;
				case INS_PLA_I: {
					A = pullFromStack(cycles, SP, memory);
					setZN(A);
				} break;
				case INS_PLP_I: {
					Byte PLP = pullFromStack(cycles, SP, memory);
					
					/*
						FINISH NOW
					*/
				} break;
                default: {
                    std::cout << "Instruction Not Handled!!! OH GOD!!!!! KJJHKHJHJHJGHGHKGJHKHGJK!!!!!!";
                } break;


            }
        }
    }

};

int main() {

    MEM mem;
    CPU cpu;
	

	
    cpu.reset(mem);
    
	cpu.X = 0x01;

	mem[0x8000] = 0xBC;
	mem[0x8001] = 0xAB;



    
    cpu.execute(4, mem);
    
    std::cout << std::hex << static_cast<int>(cpu.A) << std::endl << static_cast<int>(cpu.X) << std::endl << static_cast<int>(cpu.PC) << std::endl << static_cast<int>(cpu.Z) << std::endl << static_cast<int>(cpu.N);


    return 0;
}
