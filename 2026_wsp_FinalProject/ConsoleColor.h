#pragma once
#include <Windows.h>


class ConsoleColor
{
public :
    // static 상수로 선언
    static const int DEFAULT = 7;
    static const int GREEN = 10;
    static const int RED = 12;
    static const int YELLOW = 14;
    static const int CYAN = 11;

    //static 설정으로 바로 호출 하게 설정
    static void Set(int color);
};

