#include "input.h"
#include "Handle.h"
#include "DirectoryCheck.h"
#include "ConsoleColor.h"

#include <windows.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

struct MonitorStatistics
{
    int added = 0;
    int modified = 0;
    int removed = 0;
    int renamed = 0;
    std::unordered_map<std::string, int> changesByFile;

    void Add(const FileChangeEvent& event)
    {
        switch (event.action)
        {
        case ChangeAction::Added:
            ++added;
            break;
        case ChangeAction::Modified:
            ++modified;
            break;
        case ChangeAction::Removed:
            ++removed;
            break;
        case ChangeAction::RenamedNew:
            ++renamed;
            break;
        default:
            break;
        }

        if (event.action != ChangeAction::RenamedOld)
        {
            ++changesByFile[event.fileName];
        }
    }

    int Total() const
    {
        return added + modified + removed + renamed;
    }

    std::pair<std::string, int> MostChangedFile() const
    {
        std::pair<std::string, int> result = { "-", 0 };
        for (const auto& item : changesByFile)
        {
            if (item.second > result.second)
            {
                result = item;
            }
        }
        return result;
    }
};

static std::string FormatBytes(std::uintmax_t bytes)
{
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    double value = static_cast<double>(bytes);
    int unitIndex = 0;

    while (value >= 1024.0 && unitIndex < 4)
    {
        value /= 1024.0;
        ++unitIndex;
    }

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(unitIndex == 0 ? 0 : 2)
        << value << " " << units[unitIndex];
    return stream.str();
}

static std::string StateSize(const FileState& state)
{
    if (!state.exists)
    {
        return "-";
    }
    return state.isDirectory ? "해당 없음" : FormatBytes(state.size);
}

static std::string FileType(const FileChangeEvent& event)
{
    const FileState& state = event.after.exists ? event.after : event.before;
    if (!state.exists)
    {
        return "알 수 없음";
    }
    return state.isDirectory ? "폴더" : "파일";
}

static std::string SizeDifference(const FileChangeEvent& event)
{
    if (event.before.exists && event.before.isDirectory)
    {
        return "-";
    }
    if (event.after.exists && event.after.isDirectory)
    {
        return "-";
    }

    std::intmax_t difference = 0;
    if (!event.before.exists && event.after.exists)
    {
        difference = static_cast<std::intmax_t>(event.after.size);
    }
    else if (event.before.exists && !event.after.exists)
    {
        difference = -static_cast<std::intmax_t>(event.before.size);
    }
    else if (event.before.exists && event.after.exists)
    {
        difference =
            static_cast<std::intmax_t>(event.after.size) -
            static_cast<std::intmax_t>(event.before.size);
    }
    else
    {
        return "-";
    }

    if (difference == 0)
    {
        return "0 B";
    }

    return std::string(difference > 0 ? "+" : "-") +
        FormatBytes(static_cast<std::uintmax_t>(difference > 0 ? difference : -difference));
}

static std::string CsvEscape(const std::string& value)
{
    std::string escaped = value;
    std::size_t position = 0;
    while ((position = escaped.find('"', position)) != std::string::npos)
    {
        escaped.insert(position, 1, '"');
        position += 2;
    }
    return "\"" + escaped + "\"";
}

static std::string LogFileTimestamp()
{
    const std::time_t now = std::time(nullptr);
    std::tm localTime = {};
    localtime_s(&localTime, &now);

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y%m%d_%H%M%S");
    return stream.str();
}

static bool SaveCsvLog(
    const std::vector<FileChangeEvent>& history,
    const MonitorStatistics& statistics,
    const std::filesystem::path& watchedPath,
    std::filesystem::path& savedPath)
{
    const std::filesystem::path logDirectory =
        std::filesystem::current_path() / "CSV_Logs";
    std::error_code errorCode;
    std::filesystem::create_directories(logDirectory, errorCode);
    if (errorCode)
    {
        return false;
    }

    savedPath = logDirectory /
        ("directory_monitor_log_" + LogFileTimestamp() + ".csv");

    std::ofstream output(savedPath, std::ios::binary);
    if (!output)
    {
        return false;
    }

    output << "\xEF\xBB\xBF";
    output << "감시 디렉터리," << CsvEscape(watchedPath.u8string()) << "\r\n";
    output << "총 변경," << statistics.Total()
        << ",추가," << statistics.added
        << ",수정," << statistics.modified
        << ",삭제," << statistics.removed
        << ",이름 변경," << statistics.renamed << "\r\n\r\n";
    output << "변경 시간,작업 종류,파일 경로,구분,변경 전 크기,변경 후 크기,크기 차이,마지막 수정 시각\r\n";

    for (const auto& event : history)
    {
        const FileState& latestState = event.after.exists ? event.after : event.before;
        output << CsvEscape(event.timestamp) << ","
            << CsvEscape(event.actionStr) << ","
            << CsvEscape(event.fileName) << ","
            << CsvEscape(FileType(event)) << ","
            << CsvEscape(StateSize(event.before)) << ","
            << CsvEscape(StateSize(event.after)) << ","
            << CsvEscape(SizeDifference(event)) << ","
            << CsvEscape(latestState.lastWriteTime) << "\r\n";
    }

    return true;
}

static void PrintEvent(
    const FileChangeEvent& event,
    const std::string& targetPath)
{
    const FileState& latestState = event.after.exists ? event.after : event.before;

    ConsoleColor::Set(event.colorCode);
    std::cout << event.timestamp << " " << event.actionStr
        << targetPath << "\\" << event.fileName << "\n";
    ConsoleColor::Set(ConsoleColor::DEFAULT);
    std::cout << "  구분: " << FileType(event)
        << " | 크기: " << StateSize(event.before) << " -> " << StateSize(event.after)
        << " | 차이: " << SizeDifference(event)
        << " | 마지막 수정: " << latestState.lastWriteTime << "\n";
}

static void PrintScreen(
    const DirectoryInput& directoryInput,
    const std::string& lastFileName,
    int lastColor,
    const std::vector<FileChangeEvent>& history,
    const MonitorStatistics& statistics)
{
    system("cls");
    directoryInput.PrintDirectoryContents(lastFileName, lastColor);

    const auto mostChanged = statistics.MostChangedFile();
    std::cout << "----------------------------------------\n";
    ConsoleColor::Set(ConsoleColor::CYAN);
    std::cout << "👀 디렉터리 설정 완료 (감시 중... )\n";
    ConsoleColor::Set(ConsoleColor::DEFAULT);
    std::cout << "[통계] 총 " << statistics.Total()
        << " | 추가 " << statistics.added
        << " | 수정 " << statistics.modified
        << " | 삭제 " << statistics.removed
        << " | 이름 변경 " << statistics.renamed << "\n";
    std::cout << "[최다 변경] " << mostChanged.first
        << " (" << mostChanged.second << "회)\n";
    std::cout << "[종료] SPACE를 누르면 요약 표시 및 CSV 저장 후 종료합니다.\n";
    std::cout << "----------------------------------------\n";

    // 기존 기능대로 감시 시작 후 쌓인 모든 로그를 계속 출력합니다.
    for (std::size_t index = 0; index < history.size(); ++index)
    {
        PrintEvent(history[index], directoryInput.GetSelectedDirectory().string());
    }
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, ".UTF-8");

    DirectoryInput directoryInput;
    Handle hDir;
    DirectoryCheck dirCheck;

    if (!directoryInput.SelectDirectory())
    {
        return 1;
    }

    const std::filesystem::path targetPath = directoryInput.GetSelectedDirectory();
    std::string targetPathStr = targetPath.string();
    dirCheck.InitializeSnapshot(targetPath);

    std::string lastFileName;
    int lastColor = ConsoleColor::DEFAULT;
    std::vector<FileChangeEvent> logHistory;
    MonitorStatistics statistics;

    const std::wstring targetPathW = targetPath.wstring();
    HANDLE rawHandle = CreateFileW(
        targetPathW.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL);
    hDir = rawHandle;

    if (!hDir.isSetHandle())
    {
        std::cout << "디렉터리 핸들을 가져오지 못했습니다. 에러 코드: "
            << GetLastError() << "\n";
        return 1;
    }

    std::atomic<bool> stopRequested = false;
    std::atomic<bool> monitorFailed = false;
    std::mutex changesMutex;
    std::vector<FileChangeEvent> pendingChanges;

    // ReadDirectoryChangesW는 기존 동기식 방식으로 유지합니다.
    std::thread monitorThread([&]()
    {
        while (!stopRequested.load())
        {
            std::vector<FileChangeEvent> changes;
            if (!dirCheck.directoryTrack(hDir, targetPathStr, changes))
            {
                if (!stopRequested.load())
                {
                    monitorFailed = true;
                }
                break;
            }

            if (!changes.empty())
            {
                std::lock_guard<std::mutex> lock(changesMutex);
                pendingChanges.insert(
                    pendingChanges.end(),
                    changes.begin(),
                    changes.end());
            }
        }
    });

    PrintScreen(directoryInput, lastFileName, lastColor, logHistory, statistics);

    while (!monitorFailed.load())
    {
        std::vector<FileChangeEvent> changes;
        {
            std::lock_guard<std::mutex> lock(changesMutex);
            changes.swap(pendingChanges);
        }

        if (!changes.empty())
        {
            for (const auto& change : changes)
            {
                statistics.Add(change);
                logHistory.push_back(change);
            }

            lastFileName = changes.back().fileName;
            lastColor = changes.back().colorCode;
            PrintScreen(directoryInput, lastFileName, lastColor, logHistory, statistics);
        }

        if (GetAsyncKeyState(VK_SPACE) & 0x8000)
        {
            stopRequested = true;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    stopRequested = true;
    CancelSynchronousIo(monitorThread.native_handle());
    monitorThread.join();

    // 종료 직전에 감지된 변경도 통계와 CSV에 포함합니다.
    {
        std::lock_guard<std::mutex> lock(changesMutex);
        for (const auto& change : pendingChanges)
        {
            statistics.Add(change);
            logHistory.push_back(change);
        }
    }

    std::filesystem::path logPath;
    const bool saved = SaveCsvLog(logHistory, statistics, targetPath, logPath);

    if (saved)
    {
        std::cout << "CSV 로그 저장 완료: " << logPath.string() << "\n";
    }
    else
    {
        ConsoleColor::Set(ConsoleColor::RED);
        std::cout << "CSV 로그 파일을 저장하지 못했습니다.\n";
        ConsoleColor::Set(ConsoleColor::DEFAULT);
    }

    std::cout << "아무 키나 누르면 종료합니다.\n";
    system("pause > nul");
    return 0;
}
