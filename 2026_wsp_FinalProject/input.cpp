// input.cpp
#include <filesystem>
#include <iostream>
#include <string>
#include "input.h"

int main()
{
    std::string selectedPath;
    std::filesystem::path selectedDirectory;

    while (true)
    {
        std::cout << "사용할 디렉터리 경로를 입력하세요: ";
        std::getline(std::cin, selectedPath);

        selectedPath = RemoveQuotes(selectedPath);
        selectedDirectory = std::filesystem::path(selectedPath);

        if (IsDirectoryPath(selectedDirectory))
        {
            std::cout << "디렉터리가 정상적으로 지정되었습니다.\n";
            break;
        }

        std::cout << "오류: 입력한 경로가 존재하지 않거나 디렉터리가 아닙니다. 다시 입력해주세요.\n\n";
    }

    std::cout << "설정된 디렉터리: " << selectedDirectory.string() << "\n\n";
    std::cout << "디렉터리 내용:\n";
    PrintDirectoryContents(selectedDirectory);
}
