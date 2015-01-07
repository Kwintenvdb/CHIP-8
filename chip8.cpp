#include "chip8.h"
#include <fstream>
#include <iostream>
#include <sstream>

unsigned char Chip8::m_KeysToCheck[] = {
	'1', '2', '3', '4',
	'Q', 'W', 'E', 'R',
	'A', 'S', 'D', 'F',
	'Z', 'X', 'C', 'V'
};

void Chip8::LoadProgram(const char* fileName)
{
	std::streampos size;
	std::ifstream file(fileName, std::ios::in | std::ios::binary | std::ios::ate);

	if (file.is_open())
	{
		size = file.tellg();
		file.seekg(0, std::ios::beg);
		file.read(reinterpret_cast<char*>(m_Mem + 0x200), size);
		file.close();

		m_RegP = 0x200;
		m_StackP = 0;

		unsigned char fontset[80] =
		{
			0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
			0x20, 0x60, 0x20, 0x20, 0x70, // 1
			0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
			0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
			0x90, 0x90, 0xF0, 0x10, 0x10, // 4
			0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
			0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
			0xF0, 0x10, 0x20, 0x40, 0x40, // 7
			0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
			0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
			0xF0, 0x90, 0xF0, 0x90, 0x90, // A
			0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
			0xF0, 0x80, 0x80, 0x80, 0xF0, // C
			0xE0, 0x90, 0x90, 0x90, 0xE0, // D
			0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
			0xF0, 0x80, 0xF0, 0x80, 0x80  // F
		};
		
		for (int i = 0; i < 80; ++i)
			m_Mem[i] = fontset[i];

		m_DelayTimer = 0;
		m_SoundTimer = 0;

		OutputDebugString("The entire file content is in memory.\n");
	}
}

void CheckKey(unsigned const char& keyToCheck, unsigned char& key)
{
	if ((GetAsyncKeyState(keyToCheck) & 0x8000) != 0)
	{
		key = 1;
	}
	else key = 0;
}

void Chip8::Step()
{
	// Handle keyboard input
	HandleKeyboard();

	// Fetch opcode
	const unsigned short opcode = *reinterpret_cast<unsigned short*>(m_Mem + m_RegP);
	const unsigned short newOpcode = (opcode << 8) + (opcode >> 8); // bit shift operators
	const unsigned short operation = newOpcode & 0xF000;
	const unsigned short address = newOpcode & 0xFFF;

	// Extra variables for readability
	const unsigned short VX = (address >> 8) & 0xF;
	const unsigned short VY = (address >> 4) & 0xF;
	const unsigned short NN = address & 0xFF;

	switch (operation)
	{	
		case 0x0000:
			switch (address & 0x00F)
			{
				// Clears the screen
				case 0x0:
					ClearScreen();
					m_RegP += 2;
					break;

				// Returns from subroutine
				case 0xE:
					--m_StackP;
					m_RegP = m_Stack[m_StackP]; // Put stored address back into program pointer
					m_RegP += 2;				// Increase program pointer again (else this will loop)
					break;
			}			
			break;

		// Jumps to address (0x0NNN)
		case 0x1000:
			m_RegP = address;
			break;
		
		// Calls subroutine at NNN
		case 0x2000:
			m_Stack[m_StackP] = m_RegP;		// Store current program pointer in stack
			++m_StackP;
			m_RegP = address;				// Set program pointer to address (NNN)
			break;
			
		// Skip next opcode if VX == NN
		case 0x3000:
			if (m_Reg[VX] == NN)
				m_RegP += 4;
			else m_RegP += 2;
			break;

		// Same as above, but VX != NN
		case 0x4000:
			if (m_Reg[VX] != NN)
				m_RegP += 4;
			else m_RegP += 2;
			break;

		// Skip next opcode if VX == VY
		case 0x5000:
			if (m_Reg[VX] == m_Reg[VY])
				m_RegP += 4;
			else m_RegP += 2;
			break;

		// Sets VX to NN
		case 0x6000:
			m_Reg[VX] = NN;
			m_RegP += 2;
			break;

		// Adds NN to VX
		case 0x7000:
			m_Reg[VX] += NN;
			m_RegP += 2;
			break;

		case 0x8000:
			switch (address & 0x00F)
			{
				// Set VX to VY
				case 0x0:
					m_Reg[VX] = m_Reg[VY];
					break;

				// Set VX to VX | VY
				case 0x1:
					m_Reg[VX] |= m_Reg[VY];
					break;

				// Set VX to VX & VY
				case 0x2:
					m_Reg[VX] &= m_Reg[VY];
					break;
				
				// Set VX to VX ^ (XOR) VY
				case 0x3: 
					m_Reg[VX] ^= m_Reg[VY];
					break;

				// Adds VY to VX.
				// VF is set to 1 when there's a carry, and to 0 when there isn't
				case 0x4:
					if (m_Reg[VY] > (0xFF - m_Reg[VX]))
						m_Reg[0xF] = 1; //carry
					else m_Reg[0xF] = 0;

					m_Reg[VX] += m_Reg[VY];
					break;

				// VY is subtracted from VX
				// VF is set to 0 when there's a borrow, and 1 when there isn't
				case 0x5: 
					if (m_Reg[VY] > m_Reg[VX])
						m_Reg[0xF] = 0; // borrow
					else m_Reg[0xF] = 1;

					m_Reg[VX] -= m_Reg[VY];
					break;

				// Shifts VX right by one
				// VF is set to the value of the least significant bit of VX before the shift
				case 0x6: 
					m_Reg[0xF] = m_Reg[VX] & 0x1;
					m_Reg[VX] >>= 1;
					break;

				// Sets VX to VY minus VX
				// VF is set to 0 when there's a borrow, and 1 when there isn't				
				case 0x7: 
					if (m_Reg[VX] > m_Reg[VY])
						m_Reg[0xF] = 0; // borrow
					else m_Reg[0xF] = 1;

					m_Reg[VX] = m_Reg[VY] - m_Reg[VX];
					break;

				// Shifts VX left by one
				// VF is set to the value of the most significant bit of VX before the shift
				case 0xE: 
					m_Reg[0xF] = m_Reg[VX] >> 7;
					m_Reg[VX] <<= 1;
					break;
			}	
			m_RegP += 2;
			break;

		// Skips the next opcode if VX != VY
		case 0x9000: 
			if (m_Reg[VX] != m_Reg[VY])
				m_RegP += 4;
			else m_RegP += 2;
			break;

		// Sets I (m_RegI) to NNN (address)
		case 0xA000:
			m_RegI = address;
			m_RegP += 2;
			break;

		// Jumps to the address NNN plus V0
		case 0xB000: 
			m_RegP = address + m_Reg[0];
			break;

		// Sets VX to a random number & NN
		case 0xC000:
			m_Reg[VX] = (rand() % 0xFF) & NN;
			m_RegP += 2;
			break;

		case 0xD000:
			{
				// 0xD014: draw sprite of 8 colums (standard) x 4 (1 byte) lines
				// byte_index = (x + y * 64) / 8
				// bit_index = (x + y * 64) % 8 --> x % 8

				const int posX = m_Reg[VX];
				const int posY = m_Reg[VY];
				const int rows = address & 0x00F;

				for (int y = 0; y < rows; ++y)
				{
					for (int x = 0; x < 8; ++x)
					{
						if (m_Mem[m_RegI + y] & (0x80 >> x))
						{
							PixSet(posX + x, posY + y);
						}	
					}		
				}		

				m_RegP += 2;
			}
			break;

		// Key pressing stuff
		case 0xE000:
			switch (address & 0x0FF)
			{
				// Skips the next instruction if the key stored in VX is pressed
				case 0x9E:
					if (m_Keys[m_Reg[VX]] != 0) // pressed
						m_RegP += 4;
					else m_RegP += 2;
					break;

				// Skips the next instruction if the key stored in VX isn't pressed
				case 0xA1:
					if (m_Keys[m_Reg[VX]] == 0) // not pressed
						m_RegP += 4;
					else m_RegP += 2;
					break;
			}
			break;
		
		case 0xF000:
			switch (NN)
			{
				// Sets VX to the value of the delay timer
				case 0x07: 
					m_Reg[VX] = m_DelayTimer;
					m_RegP += 2;
					break;

				// A key press is awaited, then stored in VX
				case 0x0A:
					{
						bool keyDown = false;
						for (int i = 0; i < 16; ++i)
						{
							 if (m_Keys[i] != 0)
							 {
								 m_Reg[VX] = i;
								 keyDown = true;
							 }
						}

						if (!keyDown) return;	// Try instruction again, wait for keypress
						m_RegP += 2;			// If a key is pressed, move on					
					}
					break;

				// Sets the delay timer to VX
				case 0x15:
					m_DelayTimer = m_Reg[VX];
					m_RegP += 2;
					break;

				// Sets the sound timer to VX
				case 0x18:
					m_SoundTimer = m_Reg[VX];
					m_RegP += 2;
					break;

				// Adds VX to I
				case 0x1E: 
					// UNDOCUMENTED FEATURE
					// VF is set to 1 when range overflow (I+VX>0xFFF), and 0 when there isn't.
					if (m_RegI + m_Reg[VX] > 0xFFF)	
						m_Reg[0xF] = 1;
					else
						m_Reg[0xF] = 0;

					m_RegI += m_Reg[VX];
					m_RegP += 2;
					break;

				// Sets I to the location of the sprite for the character in VX
				case 0x29:
					m_RegI = m_Reg[VX] * 0x5;
					m_RegP += 2;
					break;

				// Stores the binary-coded decimal representation of VX 
				case 0x33:
					m_Mem[m_RegI]	  =  m_Reg[VX] / 100;
					m_Mem[m_RegI + 1] = (m_Reg[VX] / 10) % 10;
					m_Mem[m_RegI + 2] = (m_Reg[VX] % 100) % 10;
					m_RegP += 2;
					break;

				// Stores V0 to VX in memory starting at address I	
				case 0x55: 				
					for (int i = 0; i <= VX; ++i)
						m_Mem[m_RegI + i] = m_Reg[i];

					// On the original interpreter, when the operation is done, I = I + X + 1
					m_RegI += VX + 1;
					m_RegP += 2;
					break;

				// Fills V0 to VX with values from memory starting at address I
				case 0x65:
					for (int i = 0; i <= VX; ++i)
						m_Reg[i] = m_Mem[m_RegI + i];
					
					// On the original interpreter, when the operation is done, I = I + X + 1
					m_RegI += VX + 1;
					m_RegP += 2;
					break;
			}
			break;

		default:
			OutputDebugString("UNHANDLED\n");
			break;
	}

	// Update timers
	if (m_DelayTimer > 0)
		--m_DelayTimer;

	if (m_SoundTimer > 0)
	{
		OutputDebugString("BEEP\n");
		--m_SoundTimer;
	}
}

void Chip8::PixSet(const int x, const int y)
{
	const int index = x + y * 64;

	if (m_Vid[index] == 0)
		m_Vid[index] = 255; // white
	else m_Vid[index] = 0;
}

bool Chip8::PixGet(const int x, const int y) const
{
	const int index = x + y * 64;
	return (m_Vid[index] != 0);
}

void Chip8::PixClear(const int x, const int y)
{
	const int index = x + y * 64;
	m_Vid[index] = 0;
}

void Chip8::ClearScreen()
{
	for (int i = 0; i < 2048; ++i)
	{
		m_Vid[i] = 0;
	}
}

void Chip8::HandleKeyboard()
{
	for (int i = 0; i < 16; ++i)
	{
		CheckKey(m_KeysToCheck[i], m_Keys[i]);
	}
}