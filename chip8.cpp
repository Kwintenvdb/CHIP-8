#include "chip8.h"
#include <fstream>
#include <iostream>
#include <Windows.h>
#include <sstream>

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

		OutputDebugString("the entire file content is in memory\n");
	}
}

void Chip8::Step()
{
	const unsigned short opcode = *reinterpret_cast<unsigned short*>(m_Mem + m_RegP);
	const unsigned short newOpcode = (opcode << 8) + (opcode >> 8); // bit shift operators
	const unsigned short operation = newOpcode & 0xF000;
	const unsigned short address = newOpcode & 0xFFF; // value + mask (only keeps overlapping part)

	switch (operation)
	{
		case 0x0000:
			for (int x = 0; x < 64; ++x)
			{
				for (int y = 0; y < 32; ++y)
				{
					PixClear(x, y);
				}
			}

			m_RegP += 2;
			break;

		case 0x1000:
			m_RegP = address;
			break;
			
		case 0x3000:
			if (m_Reg[(address >> 8) & 0xF] == (address & 0x0FF))
				m_RegP += 4;
			else m_RegP += 2;
			break;

		case 0x6000:
			m_Reg[(address >> 8)] = (address & 0x0FF);
			m_RegP += 2;
			break;

		case 0x7000:
			m_Reg[(newOpcode >> 8) & 0xF] += (newOpcode & 0x0FF);
			m_RegP += 2;
			break;

		case 0x8000:
			switch (address & 0x00F)
			{
			case 0x0:
				m_Reg[(address >> 8) & 0xF] = (address >> 4) & 0xF;
				break;
			}
		
			m_RegP += 2;
			break;

		case 0xA000:
			m_RegI = address;
			m_RegP += 2;
			break;

		case 0xC000:
			m_Reg[(newOpcode >> 8) & 0xF] = rand() % ((address & 0xFF) + 1);
			m_RegP += 2;
			break;

		case 0xD000:
			{
				// 0xD014: draw sprite of 8 colums (standard) x 4 (1 byte) lines
				// byte_index = (x + y * 64) / 8
				// bit_index = (x + y * 64) % 8 --> x % 8

				const int posX = m_Reg[(address >> 8) & 0x00F];
				const int posY = m_Reg[(address >> 4) & 0x00F];
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

		
		case 0xF000:
			switch (address & 0xFF)
			{
			case 0x33:
				m_Mem[m_RegI] = m_Reg[(address & 0xF00) >> 8] / 100;
				m_Mem[m_RegI + 1] = (m_Reg[(address & 0xF00) >> 8] / 10) % 10;
				m_Mem[m_RegI + 2] = (m_Reg[(address & 0xF00) >> 8] % 100) % 10;
				m_RegP += 2;
				break;

			case 0x65:
				for (int i = 0; i < (address >> 8) & 0xF; ++i)
				{
					m_Reg[i] = m_Mem[m_RegI + i];
				}
				m_RegP += 2;
				break;

			case 0x29:
				m_RegI = m_Reg[(address >> 8) & 0xF] * 5;
				m_RegP += 2;
				break;

			case 0x0A:
				if (GetAsyncKeyState(VK_SPACE) != 0)
				{
					m_Reg[(address >> 8) & 0xF] = VK_SPACE;
					m_RegP += 2;
				}
				
				break;
			}
			break;

		default:
			OutputDebugString("UNHANDLED\n");
			break;
	}
}

void Chip8::PixSet(const int x, const int y)
{
	const int index = x + y * 64;
	m_Vid[index] = 255; // white
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