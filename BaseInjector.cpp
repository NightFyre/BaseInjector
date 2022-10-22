#include "pch.h"
#include "include/Initialize.h"

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR cmdLine, int cmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(cmdLine);
	UNREFERENCED_PARAMETER(cmdShow);
	return Initialize_InjectionModule(GetData());
}