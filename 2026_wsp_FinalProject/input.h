// input.h
#pragma once

#include <filesystem>
#include <iostream>
#include <string>

inline std::string RemoveQuotes(const std::string& path)
{
    if (path.length() >= 2 && path.front() == '"' && path.back() == '"')
    {
        return path.substr(1, path.length() - 2);
    }

    return path;
}

inline bool IsDirectoryPath(const std::filesystem::path& path)
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

inline void PrintDirectoryContents(const std::filesystem::path& directoryPath)
{
    std::error_code errorCode;
    std::filesystem::directory_iterator iterator(directoryPath, errorCode);

    if (errorCode)
    {
        std::cout << "오류: 디렉터리 내용을 읽을 수 없습니다.\n";
        return;
    }

    bool isEmpty = true;

    for (const auto& entry : iterator)
    {
        isEmpty = false;
        std::string name = entry.path().filename().string();

        if (entry.is_directory())
            std::cout << "[폴더] " << name << "\n";
        else if (entry.is_regular_file())
            std::cout << "[파일] " << name << "\n";
        else
            std::cout << "[기타] " << name << "\n";
    }

    if (isEmpty)
    {
        std::cout << "디렉터리 안에 파일이나 폴더가 없습니다.\n";
    }
}