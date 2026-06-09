// input.h
#pragma once
#include <iostream>
#include <system_error>
#include <filesystem>
#include <string>
#include "ConsoleColor.h"

// [수정] 디렉터리 입력, 검증, 목록 출력을 하나의 클래스로 관리합니다.
class DirectoryInput
{
public:
    // [수정] 올바른 디렉터리가 입력될 때까지 반복해서 입력받습니다.
    bool SelectDirectory();

    // [수정] 선택된 디렉터리 안의 파일과 폴더 목록을 출력합니다.
    // 🎨 [수정] 파일 이름과 색상 코드를 매개변수로 받습니다. (기본값 세팅)
    void PrintDirectoryContents(const std::string& targetFile = "", int colorCode = ConsoleColor::DEFAULT) const;

    // [수정] 이후 파일 처리에서 선택된 경로를 사용할 수 있도록 반환합니다.
    const std::filesystem::path& GetSelectedDirectory() const;

private:
    std::filesystem::path selectedDirectory;

    static std::string RemoveQuotes(const std::string& path);
    static bool IsDirectoryPath(const std::filesystem::path& path);
};
