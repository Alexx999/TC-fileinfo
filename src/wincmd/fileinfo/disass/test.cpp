#include "windows.h"
#include "stdio.h"
#include "conio.h"
#include "cadt.h"

VOID Disasm(PVOID Addr)
{
    char dBuff[1024];
	PVOID cPtr;
	ULONG Len;
	TDisCommand Command;
	TInstruction Instr;
	TMnemonicOptios Options;

    cPtr = Addr;
    Options.RealtiveOffsets = TRUE;
    Options.AddAddresPart   = TRUE;
    Options.AlternativeAddres = 0;
    Options.AddHexDump = TRUE;
    Options.MnemonicAlign = 35;
    do
	{
        memset(&Instr, 0, sizeof(TInstruction));
        memset(&Command, 0, sizeof(TDisCommand));
        Len = InstrDecode(cPtr, &Instr, FALSE);
        InstrDasm(&Instr, &Command, FALSE);
        MakeMnemonic(dBuff, &Command, &Options);
		printf("%s \n", dBuff);
        cPtr = (PVOID)((ULONG_PTR)cPtr + Len);
	} while (Command.CmdOrdinal != 0x4C);

	return;
}


int main(int argc, char* argv[])
{
	printf("%s \n\n", GetCadtVersion());
	Disasm(GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA"));
	getch();
	return 0;
}

