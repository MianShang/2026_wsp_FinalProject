#include "ConsoleColor.h"

void ConsoleColor::Set(int color)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}