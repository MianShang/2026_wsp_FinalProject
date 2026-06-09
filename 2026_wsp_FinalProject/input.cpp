// input.cpp
#include "input.h"

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
void DirectoryInput::PrintDirectoryContents(const std::string& targetFile, int colorCode) const
{
    std::error_code errorCode;
    std::filesystem::directory_iterator iterator(selectedDirectory, errorCode);

    if (errorCode) { std::cout << "오류: 디렉터리 내용을 읽을 수 없습니다.\n"; return; }

    std::cout << "디렉터리 내용:\n";
    bool isEmpty = true;

    // 💡 핵심: 감지된 파일(targetFile)이 루트(selectedDirectory) 기준으로 어디에 있는지 계산
    // 예: 루트가 D:\Project 이고 변경된게 D:\Project\Sub\test.txt 라면
    // relative는 "Sub\test.txt"가 되고, 여기서 첫 번째 요소는 "Sub"가 됩니다.
    std::filesystem::path root(selectedDirectory);
    std::filesystem::path changedPath(targetFile); // 만약 targetFile이 전체 경로라면

    // 상대 경로 계산 (상대 경로의 가장 첫 번째 항목이 현재 리스트에 보여지는 이름)
    std::string targetHighlightName = "";
    if (changedPath.is_absolute()) {
        std::filesystem::path relative = changedPath.lexically_relative(root);
        if (!relative.empty()) {
            targetHighlightName = (*relative.begin()).string();
        }
    }
    else {
        // targetFile이 이미 파일명/상대경로라면 그대로 사용
        targetHighlightName = changedPath.begin()->string();
    }

    for (const auto& entry : iterator)
    {
        isEmpty = false;
        std::string currentName = entry.path().filename().string();

        // 🌟 이제 리스트에 출력되는 이름(currentName)과 
        // 변경된 파일이 속한 최상위 항목 이름(targetHighlightName)을 비교!
        if (currentName == targetHighlightName) {
            ConsoleColor::Set(colorCode);
        }
        else {
            ConsoleColor::Set(ConsoleColor::DEFAULT);
        }

        if (entry.is_directory()) std::cout << "[폴더] " << currentName << "\n";
        else std::cout << "[파일] " << currentName << "\n";

        ConsoleColor::Set(ConsoleColor::DEFAULT);
    }

    if (isEmpty) { std::cout << "디렉터리 안에 파일이나 폴더가 없습니다.\n"; }
}

const std::filesystem::path& DirectoryInput::GetSelectedDirectory() const
{
    return selectedDirectory;
}
