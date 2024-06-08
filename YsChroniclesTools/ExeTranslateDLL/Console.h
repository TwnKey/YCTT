#ifndef PROGRESS
#define PROGRESS

#include <iostream>
#include <windows.h>


class Console
{
private:
	FILE* f;
	FILE* f2;
	float progress = 0;
	int barWidth = 70;
	FILE m_OldStdin, m_OldStdout;

public:
	Console();
	~Console();
	bool Greetings();
	float GetProgress();
	bool Update(float progress);
	void ControlBackToTheUser();

};

#endif // !PROGRESS