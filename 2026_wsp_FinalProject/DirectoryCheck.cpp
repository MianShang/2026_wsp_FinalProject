#include "DirectoryCheck.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>

std::string DirectoryCheck::NormalizeRelativePath(const std::string& path)
{
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

std::string DirectoryCheck::CurrentTime()
{
    const std::time_t now = std::time(nullptr);
    std::tm localTime = {};
    localtime_s(&localTime, &now);

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}

std::string DirectoryCheck::FileTimeToString(const std::filesystem::file_time_type& time)
{
    const auto systemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    const std::time_t converted = std::chrono::system_clock::to_time_t(systemTime);
    std::tm localTime = {};
    localtime_s(&localTime, &converted);

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}

FileState DirectoryCheck::ReadFileState(const std::filesystem::path& path)
{
    FileState state;
    std::error_code errorCode;

    state.exists = std::filesystem::exists(path, errorCode);
    if (errorCode || !state.exists)
    {
        return state;
    }

    state.isDirectory = std::filesystem::is_directory(path, errorCode);
    if (errorCode)
    {
        return state;
    }

    if (!state.isDirectory)
    {
        state.size = std::filesystem::file_size(path, errorCode);
        if (errorCode)
        {
            state.size = 0;
            errorCode.clear();
        }
    }

    const auto lastWrite = std::filesystem::last_write_time(path, errorCode);
    if (!errorCode)
    {
        state.lastWriteTime = FileTimeToString(lastWrite);
    }

    return state;
}

void DirectoryCheck::InitializeSnapshot(const std::filesystem::path& path)
{
    rootPath = path;
    fileStates.clear();

    std::error_code errorCode;
    std::filesystem::recursive_directory_iterator iterator(
        rootPath,
        std::filesystem::directory_options::skip_permission_denied,
        errorCode);
    const std::filesystem::recursive_directory_iterator end;

    while (!errorCode && iterator != end)
    {
        const std::filesystem::path relative = iterator->path().lexically_relative(rootPath);
        fileStates[NormalizeRelativePath(relative.generic_u8string())] = ReadFileState(iterator->path());
        iterator.increment(errorCode);
    }
}

void DirectoryCheck::ProcessBuffer(DWORD length, std::vector<FileChangeEvent>& outEvents)
{
    if (length == 0)
    {
        return;
    }

    FILE_NOTIFY_INFORMATION* event = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);

    while (true)
    {
        const std::wstring wideName(event->FileName, event->FileNameLength / sizeof(WCHAR));
        const int requiredSize = WideCharToMultiByte(
            CP_UTF8, 0, wideName.c_str(), static_cast<int>(wideName.size()),
            NULL, 0, NULL, NULL);

        std::string fileName(requiredSize, '\0');
        if (requiredSize > 0)
        {
            WideCharToMultiByte(
                CP_UTF8, 0, wideName.c_str(), static_cast<int>(wideName.size()),
                fileName.data(), requiredSize, NULL, NULL);
        }

        const std::string key = NormalizeRelativePath(fileName);

        if (key.find("$RECYCLE.BIN") == std::string::npos &&
            key.find("System Volume Information") == std::string::npos)
        {
            FileChangeEvent change;
            change.timestamp = CurrentTime();
            change.fileName = fileName;

            const auto previous = fileStates.find(key);
            if (previous != fileStates.end())
            {
                change.before = previous->second;
            }

            const std::filesystem::path fullPath = rootPath / std::filesystem::u8path(key);

            switch (event->Action)
            {
            case FILE_ACTION_ADDED:
                change.action = ChangeAction::Added;
                change.actionStr = "[파일 추가] ";
                change.colorCode = ConsoleColor::GREEN;
                change.after = ReadFileState(fullPath);
                fileStates[key] = change.after;
                break;

            case FILE_ACTION_MODIFIED:
                change.action = ChangeAction::Modified;
                change.actionStr = "[파일 수정] ";
                change.colorCode = ConsoleColor::YELLOW;
                change.after = ReadFileState(fullPath);
                fileStates[key] = change.after;
                break;

            case FILE_ACTION_REMOVED:
                change.action = ChangeAction::Removed;
                change.actionStr = "[파일 삭제] ";
                change.colorCode = ConsoleColor::RED;
                fileStates.erase(key);
                break;

            case FILE_ACTION_RENAMED_OLD_NAME:
                change.action = ChangeAction::RenamedOld;
                change.actionStr = "[파일 예전 이름] ";
                change.colorCode = ConsoleColor::CYAN;
                renamedOldState = change.before;
                hasRenamedOldState = true;
                fileStates.erase(key);
                break;

            case FILE_ACTION_RENAMED_NEW_NAME:
                change.action = ChangeAction::RenamedNew;
                change.actionStr = "[파일 새 이름] ";
                change.colorCode = ConsoleColor::CYAN;
                if (hasRenamedOldState)
                {
                    change.before = renamedOldState;
                    hasRenamedOldState = false;
                }
                change.after = ReadFileState(fullPath);
                fileStates[key] = change.after;
                break;

            default:
                change.actionStr = "[기타 변경]";
                change.after = ReadFileState(fullPath);
                break;
            }

            outEvents.push_back(change);
        }

        if (event->NextEntryOffset == 0)
        {
            break;
        }

        event = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
            reinterpret_cast<PBYTE>(event) + event->NextEntryOffset);
    }
}

BOOL DirectoryCheck::directoryTrack(
    Handle& hDir,
    std::string& path,
    std::vector<FileChangeEvent>& outEvents)
{
    if (FALSE == hDir.isSetHandle())
    {
        ConsoleColor::Set(ConsoleColor::RED);
        std::cout << "Handle Set Error : " << __FUNCTION__ << ", " << GetLastError() << "\n";
        ConsoleColor::Set(ConsoleColor::DEFAULT);
        return FALSE;
    }

    BYTE buffer[2048] = {};
    DWORD length = 0;
    if (FALSE == ReadDirectoryChangesW(
        hDir.getHandle(),
        buffer,
        sizeof(buffer),
        TRUE,
        flags,
        &length,
        NULL,
        NULL))
    {
        if (GetLastError() == ERROR_OPERATION_ABORTED)
        {
            return FALSE;
        }

        ConsoleColor::Set(ConsoleColor::RED);
        std::cout << "ReadDirectoryChangesW : " << GetLastError() << "\n";
        ConsoleColor::Set(ConsoleColor::DEFAULT);
        hDir.closeHandle();
        return FALSE;
    }

    // 기존 인터페이스의 경로 인수를 유지합니다.
    (void)path;

    // 기존 방식과 동일하게 동기 호출로 변경 이벤트를 처리합니다.
    // ProcessBuffer가 사용하는 멤버 버퍼로 결과를 복사합니다.
    std::memcpy(this->buffer, buffer, length);
    ProcessBuffer(length, outEvents);
    return TRUE;
}
