#include "Console.h"
#include <windows.h>
#include <fstream>

static BOOL WINAPI MyConsoleCtrlHandler(DWORD dwCtrlEvent) { return dwCtrlEvent == CTRL_C_EVENT; }


void ShowConsoleCursor(bool showFlag)
{
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_CURSOR_INFO     cursorInfo;

	GetConsoleCursorInfo(out, &cursorInfo);
	cursorInfo.bVisible = showFlag; // set the cursor visibility
	SetConsoleCursorInfo(out, &cursorInfo);
}

void Console::ControlBackToTheUser()
{
	ShowConsoleCursor(true);
}

Console::Console() {

	f = new FILE();
	f2 = new FILE();

	AllocConsole();

	freopen_s(&f2, "CONIN$", "r", stdin);
	freopen_s(&f, "CONOUT$", "w", stderr);
	freopen_s(&f, "CONOUT$", "w", stdout);

}


Console::~Console() {

	fprintf(stdout, "Closing console.\n");
	fclose(stdout);
	fclose(stdin);
	fclose(stderr);
	*stdout = m_OldStdout;
	*stdin = m_OldStdin;
	SetConsoleCtrlHandler(MyConsoleCtrlHandler, false);
	delete f;
	delete f2;
	FreeConsole();
	
}

float Console::GetProgress() {
	return progress;
}
bool Console::Update(float progress) {
	fprintf(stdout, "[");
	int pos = barWidth * progress;
	for (int i = 0; i < barWidth; ++i) {
		if (i < pos) fprintf(stdout, "=");
		else if (i == pos) fprintf(stdout, ">");
		else fprintf(stdout, " ");
	}
	fprintf(stdout, "] %d %% \r", int(progress * 100.0));
	fflush(stdout);
	return true;
}

bool Console::Greetings() {
	//fprintf(stdout, "Translation patch.\n");
	return true;
}