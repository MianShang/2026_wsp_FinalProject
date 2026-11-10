#pragma once

#include <Windows.h>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "ConsoleColor.h"
#include "Handle.h"

enum class ChangeAction
{
    Added,
    Modified,
    Removed,
    RenamedOld,
    RenamedNew,
    Unknown
};

struct FileState
{
    bool exists = false;
    bool isDirectory = false;
    std::uintmax_t size = 0;
    std::string lastWriteTime = "-";
};

struct FileChangeEvent
{
    std::string timestamp;
    std::string fileName;
    std::string actionStr;
    ChangeAction action = ChangeAction::Unknown;
    int colorCode = ConsoleColor::DEFAULT;
    FileState before;
    FileState after;
};

class DirectoryCheck
{
private:
    DWORD flags = FILE_NOTIFY_CHANGE_FILE_NAME |
        FILE_NOTIFY_CHANGE_DIR_NAME |
        FILE_NOTIFY_CHANGE_ATTRIBUTES |
        FILE_NOTIFY_CHANGE_SIZE |
        FILE_NOTIFY_CHANGE_LAST_WRITE |
        FILE_NOTIFY_CHANGE_CREATION;

    BYTE buffer[2048] = {};
    std::filesystem::path rootPath;
    std::unordered_map<std::string, FileState> fileStates;
    FileState renamedOldState;
    bool hasRenamedOldState = false;

    static std::string NormalizeRelativePath(const std::string& path);
    static std::string CurrentTime();
    static std::string FileTimeToString(const std::filesystem::file_time_type& time);
    static FileState ReadFileState(const std::filesystem::path& path);
    void ProcessBuffer(DWORD length, std::vector<FileChangeEvent>& outEvents);

public:
    void InitializeSnapshot(const std::filesystem::path& path);
    BOOL directoryTrack(
        Handle& hDir,
        std::string& path,
        std::vector<FileChangeEvent>& outEvents);
};
