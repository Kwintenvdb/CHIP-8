#include <Windows.h>

struct Chip8
{
	void LoadProgram(const char* fileName);
	void Step();
	void PixSet(const int x, const int y);
	bool PixGet(const int x, const int y) const;
	void PixClear(const int x, const int y);
	void ClearScreen();
	void HandleKeyboard();

	unsigned char m_Mem[4096];		 // Memory
	unsigned char m_Vid[2048];		 // Pixels: one byte per pixel
	unsigned char m_Reg[16];		 // V-registers
	unsigned short m_Stack[16];		 // Stack: used to store subroutines

	unsigned short m_RegI;			 // Index register
	unsigned short m_RegP;			 // Register pointer
	unsigned short m_StackP;		 // Stack pointer

	unsigned char m_DelayTimer;		 // Used for timing events in games
	unsigned char m_SoundTimer;		 // If nonzero, beeps

	unsigned char m_Keys[16];		 // Stores key states: 0 or 1
	static unsigned char m_KeysToCheck[]; // Keyboard layout
};