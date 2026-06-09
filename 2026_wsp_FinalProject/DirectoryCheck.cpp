#include "DirectoryCheck.h"


BOOL DirectoryCheck::directoryTrack(Handle& hDir, std::string& path, std::vector<FileChangeEvent>& outEvents)
{
	if (FALSE == hDir.isSetHandle())
	{
		ConsoleColor::Set(ConsoleColor::RED);
		std::cout << "Handle Set Error : " << __FUNCTION__ << ", " << GetLastError() << "\n";
		ConsoleColor::Set(ConsoleColor::DEFAULT);
		return FALSE;
	}

	/// (화면 출력 로직을 제거하고 데이터 수집에 집중합니다)
	BYTE bData[2048];
	DWORD len;

	if (FALSE == ReadDirectoryChangesW(
		hDir.getHandle(),       /// 감시 디렉터리 핸들
		bData,                  /// 정보가 남겨져 올 버퍼
		sizeof(bData),          /// 버퍼의 크기 (안전성을 위해 sizeof 사용)
		TRUE,                   /// 하위 디렉터리 검사 여부
		flags,                  /// 검사 종류
		&len,                   /// 사용한 버퍼의 길이
		NULL,                   /// Overlapped 구조체 : 비동기
		NULL                    /// APC 이름 : 비동기
	))
	{
		ConsoleColor::Set(ConsoleColor::RED);
		std::cout << "ReadDirectoryChangesW : " << GetLastError() << "\n";
		ConsoleColor::Set(ConsoleColor::DEFAULT);
		hDir.closeHandle();
		return FALSE;
	}

	/// 변경된 정보가 정상적으로 도착했다.
	FILE_NOTIFY_INFORMATION* event = (FILE_NOTIFY_INFORMATION*)bData;

	while (TRUE)
	{
		/// 유니코드(WCHAR) 배열을 길이에 맞춰 wstring으로 변환 (글자 증발 방지)
		std::wstring wFileName(event->FileName, event->FileNameLength / sizeof(WCHAR));

		/// 윈도우 API를 사용해 UTF-8 string으로 변환 (한글 깨짐 방지)
		int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wFileName.c_str(), -1, NULL, 0, NULL, NULL);
		std::string utf8FileName(sizeNeeded > 0 ? sizeNeeded - 1 : 0, 0);
		if (sizeNeeded > 0) {
			WideCharToMultiByte(CP_UTF8, 0, wFileName.c_str(), -1, &utf8FileName[0], sizeNeeded, NULL, NULL);
		}

		if (utf8FileName.find("$RECYCLE.BIN") != std::string::npos ||
			utf8FileName.find("System Volume Information") != std::string::npos) {

			if (0 == event->NextEntryOffset) break;
			event = (FILE_NOTIFY_INFORMATION*)((PBYTE)event + event->NextEntryOffset);
			continue; /// 아래의 출력 로직을 타지 않고 다음 이벤트로 진행
		}

		// 결과 전달을 위한 구조체 생성
		FileChangeEvent change;
		change.fileName = utf8FileName;
		change.colorCode = ConsoleColor::DEFAULT;


		int actionColor = ConsoleColor::DEFAULT;

		switch (event->Action)
		{
		case FILE_ACTION_ADDED:    /// 새로운 파일이 생성
			change.actionStr = "[파일 추가] ";
			change.colorCode = ConsoleColor::GREEN;
			break;

		case FILE_ACTION_MODIFIED: /// 존재하는 파일이 수정
			change.actionStr = "[파일 수정] ";
			change.colorCode = ConsoleColor::YELLOW;
			break;

		case FILE_ACTION_REMOVED:   /// 존재하는 파일이 삭제
			change.actionStr = "[파일 삭제] ";
			change.colorCode = ConsoleColor::RED;
			break;

			/// 파일 이름이 변경된 경우
		case FILE_ACTION_RENAMED_NEW_NAME:  /// 새로운 파일 이름
			change.actionStr = "[파일 새 이름] ";
			change.colorCode = ConsoleColor::CYAN;
			break;

		case FILE_ACTION_RENAMED_OLD_NAME:  /// 이전 파일 이름
			change.actionStr = "[파일 예전 이름] ";
			change.colorCode = ConsoleColor::CYAN;
			break;
		}

		/// 감지된 이벤트를 vector에 추가하여 호출부로 전달
		outEvents.push_back(change);

		/// 무한 반복을 종료시키는 위치
		if (0 == event->NextEntryOffset)
			break;
		/// NextEntryOffset이 0이 아니라는 의미 => 다음 데이터가 있다.
		event = (FILE_NOTIFY_INFORMATION*)((PBYTE)event + event->NextEntryOffset);
	}

	return TRUE;
}