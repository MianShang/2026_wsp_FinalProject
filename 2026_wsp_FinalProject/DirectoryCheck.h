#pragma once
#include <iostream>
#include <Windows.h>
#include <string>
#include "Handle.h"
#include <vector>
#include "ConsoleColor.h"

struct FileChangeEvent {
    std::string fileName;
    std::string actionStr;
    int colorCode;
};

class DirectoryCheck
{
private :

    /// 디렉터리 변경에 대한 이벤트를 받을 Actions 목록 설정
    DWORD flags = FILE_NOTIFY_CHANGE_FILE_NAME |      /// 파일 이름 변경
        FILE_NOTIFY_CHANGE_DIR_NAME |                 /// 디렉터리 이름 변경
        FILE_NOTIFY_CHANGE_ATTRIBUTES |               /// 파일 속성 변경
        FILE_NOTIFY_CHANGE_SIZE |                     /// 파일 크기 변경
        FILE_NOTIFY_CHANGE_LAST_WRITE |               /// 마지막 수정 발생
        FILE_NOTIFY_CHANGE_CREATION;                  /// 파일 생성

public :
	
	BOOL directoryTrack(Handle&, std::string&, std::vector<FileChangeEvent>&);

};

