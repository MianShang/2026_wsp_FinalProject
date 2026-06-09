#include "input.h"
#include "Handle.h"
#include "DirectoryCheck.h"
#include "ConsoleColor.h" // main에서도 색상을 쓰기 위해 추가
#include <windows.h>
#include <iostream>
#include <vector>

int main()
{
    //Windows CMD 창에서 한글이 깨지지 않도록 UTF-8 코드 페이지를 설정합니다.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, ".UTF-8");

    DirectoryInput directoryInput;
    Handle hDir;
    DirectoryCheck dirCheck;

    // 디렉터리 경로 입력
    if (!directoryInput.SelectDirectory())
    {
        return 1;
    }

    // DirectoryCheck의 directoryTrack 함수에 넘겨줄 일반 string 경로
    std::string targetPathStr = directoryInput.GetSelectedDirectory().string();

    // UI 하이라이트를 위해 기억해둘 변수
    std::string lastFileName = "";
    int lastColor = ConsoleColor::DEFAULT;

    // 로그를 누적해서 저장할 벡터 (화면에서 로그가 사라지지 않게 하기 위함)
    std::vector<FileChangeEvent> logHistory;

    // 3. 본격적인 파일 감시 루프 시작
    while (true)
    {
        // 1. 대기 화면 출력 (루프마다 전체 화면을 지우지 않고 상단부만 갱신하는 느낌으로 구성)
        system("cls");
        directoryInput.PrintDirectoryContents(lastFileName, lastColor);
        std::cout << "----------------------------------------\n";
        ConsoleColor::Set(ConsoleColor::CYAN);
        std::cout << "👀 디렉터리 설정 완료 (감시 중... )\n\n";
        ConsoleColor::Set(ConsoleColor::DEFAULT);

        // 이전까지 쌓인 모든 로그 기록을 출력
        for (const auto& c : logHistory) {
            ConsoleColor::Set(c.colorCode);
            std::cout << c.actionStr << targetPathStr << "\\" << c.fileName << "\n";
        }
        ConsoleColor::Set(ConsoleColor::DEFAULT);

        // 핸들체크
        if (!hDir.isSetHandle())
        {
            // Win32 API(CreateFileW)는 유니코드(wstring)를 원하므로 변환해서 전달
            std::wstring targetPathW = directoryInput.GetSelectedDirectory().wstring();

            HANDLE rawHandle = CreateFileW(
                targetPathW.c_str(),
                FILE_LIST_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS, // 디렉터리 열기 필수 플래그
                NULL
            );

            // = 오버로딩을 통해 안전한 Handle 객체에 포장
            hDir = rawHandle;

            if (!hDir.isSetHandle())
            {
                std::cout << "디렉터리 핸들을 가져오지 못했습니다. 에러 코드 : " << GetLastError() << "\n";
                break;
            }
        }

        // 2. 구조체를 담을 vector 준비 및 감시 시작
        std::vector<FileChangeEvent> changes;

        // 여기서 프로그램이 멈춰 있다가, 파일이 바뀌면 changes에 데이터가 담겨서 내려옵니다!
        if (dirCheck.directoryTrack(hDir, targetPathStr, changes))
        {
            if (!changes.empty()) {
                // 가장 마지막에 변경된 파일 정보로 변수 갱신
                lastFileName = changes.back().fileName;
                lastColor = changes.back().colorCode;

                // 감지된 새 이벤트들을 전체 로그 히스토리에 추가 (누적!)
                logHistory.insert(logHistory.end(), changes.begin(), changes.end());
            }
        }

        // 3. 실시간 ESC 키 입력 감지 (엔터 대기 없이 감시 유지)
        if (GetAsyncKeyState(VK_SPACE) & 0x8000)
        {
            std::cout << "\nESC가 눌렸습니다. 프로그램을 종료합니다.\n";
            break;
        }

        // CPU 점유율 방지를 위한 짧은 딜레이
        Sleep(100);
    }

    return 0;
}