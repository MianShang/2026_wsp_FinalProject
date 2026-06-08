// 2026_wsp_FinalProject.cpp : 이 파일에는 'main' 함수가 포함됩니다.
//

#include "input.h"
#include <windows.h>

int main()
{
    // [수정] Windows CMD 창에서 한글이 깨지지 않도록 UTF-8 코드 페이지를 설정합니다.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // [수정] 디렉터리 입력과 목록 출력을 DirectoryInput 클래스를 통해 처리합니다.
    DirectoryInput directoryInput;

    if (!directoryInput.SelectDirectory())
    {
        return 1;
    }

    directoryInput.PrintDirectoryContents();

    // TODO: 이후 파일 처리 코드에서는 directoryInput.GetSelectedDirectory()를 사용하면 됩니다.
    return 0;
}
