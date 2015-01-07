struct Chip8
{
	void LoadProgram(const char* fileName);
	void Step();
	void PixSet(const int x, const int y);
	bool PixGet(const int x, const int y) const;
	void PixClear(const int x, const int y);

	unsigned char m_Mem[4096];
	unsigned char m_Vid[2048]; // one byte per pixel
	unsigned char m_Reg[16];
	unsigned short m_RegI;
	unsigned short m_RegP;

	unsigned char m_Fontset[80];
};