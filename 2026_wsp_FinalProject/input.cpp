// input.cpp
#include "input.h"

#include <iostream>
#include <system_error>

// [수정] 사용자가 경로를 따옴표로 감싸서 입력한 경우 따옴표를 제거합니다.
std::string DirectoryInput::RemoveQuotes(const std::string& path)
{
    if (path.length() >= 2 && path.front() == '"' && path.back() == '"')
    {
        return path.substr(1, path.length() - 2);
    }

    return path;
}

// [수정] 입력받은 경로가 실제로 존재하는 디렉터리인지 확인합니다.
bool DirectoryInput::IsDirectoryPath(const std::filesystem::path& path)
{
    std::error_code errorCode;
    bool exists = std::filesystem::exists(path, errorCode);

    if (errorCode || !exists)
    {
        return false;
    }

    bool isDirectory = std::filesystem::is_directory(path, errorCode);
    return !errorCode && isDirectory;
}

// [수정] 디렉터리를 입력받고 클래스 멤버에 저장합니다.
bool DirectoryInput::SelectDirectory()
{
    std::string inputPath;

    while (true)
    {
        std::cout << "사용할 디렉터리 경로를 입력하세요: ";

        if (!std::getline(std::cin, inputPath))
        {
            std::cout << "오류: 경로 입력을 읽을 수 없습니다.\n";
            return false;
        }

        inputPath = RemoveQuotes(inputPath);
        std::filesystem::path directoryPath(inputPath);

        if (IsDirectoryPath(directoryPath))
        {
            selectedDirectory = directoryPath;
            std::cout << "디렉터리가 정상적으로 지정되었습니다.\n";
            std::cout << "설정된 디렉터리: " << selectedDirectory.string() << "\n\n";
            return true;
        }

        std::cout << "오류: 입력한 경로가 존재하지 않거나 디렉터리가 아닙니다. 다시 입력해주세요.\n\n";
    }
}

// [수정] 선택된 디렉터리 안의 파일과 폴더 목록을 구분해서 출력합니다.
void DirectoryInput::PrintDirectoryContents() const
{
    std::error_code errorCode;
    std::filesystem::directory_iterator iterator(selectedDirectory, errorCode);

    if (errorCode)
    {
        std::cout << "오류: 디렉터리 내용을 읽을 수 없습니다.\n";
        return;
    }

    std::cout << "디렉터리 내용:\n";

    bool isEmpty = true;

    for (const auto& entry : iterator)
    {
        isEmpty = false;
        std::string name = entry.path().filename().string();

        if (entry.is_directory())
        {
            std::cout << "[폴더] " << name << "\n";
        }
        else if (entry.is_regular_file())
        {
            std::cout << "[파일] " << name << "\n";
        }
        else
        {
            std::cout << "[기타] " << name << "\n";
        }
    }

    if (isEmpty)
    {
        std::cout << "디렉터리 안에 파일이나 폴더가 없습니다.\n";
    }
}

const std::filesystem::path& DirectoryInput::GetSelectedDirectory() const
{
    return selectedDirectory;
}
