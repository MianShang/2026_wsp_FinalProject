<div align="center">

# 📂 실시간 디렉터리 감시 프로그램

**2026 유한대학교 윈도우 프로그래밍 기말고사 프로젝트**

![C++](https://img.shields.io/badge/C++17-00599C?style=flat-square&logo=cplusplus&logoColor=white)
![Windows](https://img.shields.io/badge/Win32_API-0078D6?style=flat-square&logo=windows&logoColor=white)
![Visual Studio](https://img.shields.io/badge/Visual_Studio-5C2D91?style=flat-square&logo=visualstudio&logoColor=white)

</div>

---

## 📑 목차
1. [프로젝트 소개](#-프로젝트-소개)
2. [주요 기능](#-주요-기능)
3. [실행 방법](#-실행-방법)
4. [프로젝트 구조](#-프로젝트-구조)
5. [팀원 및 담당 역할](#-팀원-및-담당-역할)
6. [개발 이력](#-개발-이력)

---

## 💡 프로젝트 소개

> 지정한 디렉터리를 **실시간으로 감시**하여 파일·폴더의 변경을 콘솔에 보여주고,
> 종료 시 변경 이력과 통계를 **CSV 로그**로 저장하는 Windows 콘솔 프로그램입니다.

```
경로 입력 ──▶ 디렉터리 검증 ──▶ 실시간 감시 ──▶ 변경 표시 + 통계 ──▶ SPACE ──▶ CSV 저장
```

---

## ✨ 주요 기능

| 기능 | 설명 |
|---|---|
| 📥 **경로 입력 및 검증** | 올바른 디렉터리가 입력될 때까지 반복해서 입력받음 (따옴표로 감싼 경로도 처리) |
| 📋 **목록 출력** | 감시 중인 디렉터리의 파일·폴더 목록 표시, 마지막으로 변경된 항목 하이라이트 |
| 👀 **실시간 감지** | `ReadDirectoryChangesW`로 하위 폴더까지 감시 |
| 🔍 **상세 정보** | 파일/폴더 구분, 변경 전·후 크기, 크기 차이, 마지막 수정 시각 |
| 📊 **통계** | 작업 종류별 변경 횟수, 가장 많이 변경된 파일 |
| 💾 **CSV 로그** | 종료 시 `CSV_Logs/` 폴더에 저장 (Excel에서 한글이 깨지지 않는 UTF-8 BOM) |
| 🔒 **핸들 관리** | `HANDLE`을 클래스로 감싸 소멸 시 자동으로 닫음 (RAII) |

### 🎨 변경 종류별 색상

| 변경 종류 | 출력 예시 | 색상 |
|---|---|:---:|
| 추가 | `[파일 추가]` | 🟩 초록 |
| 수정 | `[파일 수정]` | 🟨 노랑 |
| 삭제 | `[파일 삭제]` | 🟥 빨강 |
| 이름 변경 | `[파일 예전 이름]` → `[파일 새 이름]` | 🟦 하늘 |

---

## 🚀 실행 방법

1. Visual Studio에서 `2026_wsp_FinalProject/2026_wsp_FinalProject.sln`을 열고 빌드합니다.
2. 프로그램을 실행하고 감시할 디렉터리 경로를 입력합니다.
3. 해당 디렉터리에서 파일을 추가·수정·삭제·이름 변경하면 화면에 바로 표시됩니다.
4. **`SPACE`** 키를 누르면 감시를 끝내고 CSV 로그를 저장합니다.

```
📁 CSV_Logs/directory_monitor_log_YYYYMMDD_HHMMSS.csv
```

---

## 🗂 프로젝트 구조

```
2026_wsp_FinalProject/
├── 2026_wsp_FinalProject.cpp   # main, 감시 스레드, 화면 출력, 통계, CSV 저장
├── Handle.h / .cpp             # HANDLE을 자동으로 닫아주는 RAII 래퍼
├── input.h / .cpp              # DirectoryInput : 경로 입력 · 검증 · 목록 출력
├── DirectoryCheck.h / .cpp     # DirectoryCheck : 변경 감지 · 파일 상태 스냅샷
├── ConsoleColor.h / .cpp       # 콘솔 글자 색상 설정
└── CSV_Logs/                   # 실행 결과 로그
```

---

## 👥 팀원 및 담당 역할

| 팀원 | 역할 | 담당 모듈 |
|:---:|---|---|
| **임지섭** | 프로젝트 구성 · 디렉터리 감시 · 통합 | `DirectoryCheck`, `ConsoleColor`, `main` |
| **임성용** | 핸들 관리 (리소스 안전성) | `Handle` |
| **성윤우** | 입력 처리 · 감지 고도화 · 로그 | `DirectoryInput`, `DirectoryCheck`, `main` |

### 🧑‍💻 임지섭
- 저장소 및 Visual Studio 프로젝트 초기 세팅
- `DirectoryCheck` 클래스 생성, `ReadDirectoryChangesW` 기반 감시 루프 구현
- `ConsoleColor` 클래스 추가, 변경된 항목 색상 하이라이트
- 변경 로그가 화면에 계속 쌓이도록 출력 구성
- `handle_check` 브랜치 병합, 브랜치 간 충돌 해결 및 전체 병합 관리

### 🧑‍💻 임성용
- `Handle` 클래스 설계 및 구현 (`Handle.h`, `Handle.cpp`)
  - Win32 `HANDLE`을 클래스로 감싸, 객체가 사라질 때 핸들이 자동으로 닫히도록 구성 (RAII)
  - 핸들 누수와 잘못된 핸들 사용을 막는 공통 기반 제공
- 잘못된 핸들 방지 로직
  - 생성자와 `=` 연산자에서 `NULL`, `INVALID_HANDLE_VALUE`가 들어오면 저장하지 않도록 검사
  - 기본 생성자에서 `INVALID_HANDLE_VALUE`로 초기화해 쓰레기 값 사용 방지
- 핸들 관리용 메서드 제공

  | 메서드 | 기능 |
  |---|---|
  | `operator=` | `hDir = CreateFileW(...)`처럼 대입만으로 핸들 등록 |
  | `isSetHandle()` | 현재 유효한 핸들을 가지고 있는지 확인 |
  | `getHandle()` | Win32 API 호출에 넘길 원본 핸들 반환 |
  | `closeHandle()` | 핸들을 닫고 초기 상태로 되돌림 |

- 프로그램에서의 활용
  - `main`에서 감시 대상 디렉터리 핸들을 `Handle` 객체로 관리
  - `DirectoryCheck::directoryTrack()`이 `isSetHandle()`로 핸들을 확인한 뒤 `getHandle()`로 `ReadDirectoryChangesW`에 전달
  - 오류가 나면 `closeHandle()`로 핸들 정리

### 🧑‍💻 성윤우
- `DirectoryInput` 클래스 구현
  - 경로 입력, 따옴표 제거, 디렉터리 검증, 목록 출력
- 파일 상태 스냅샷으로 변경 전·후 크기 비교
- 감시 작업을 별도 스레드로 분리
- 작업별 통계 및 최다 변경 파일 집계
- CSV 로그 저장 기능

---

## 📅 개발 이력

| 날짜 | 작성자 | 내용 |
|:---:|:---:|---|
| 06-02 | 임지섭 | 저장소 생성 및 프로젝트 추가 |
| 06-08 | 임성용 | `Handle` 클래스 추가 (핸들 자동 해제 · 유효성 검사 · 대입 연산자 오버로딩) |
| 06-08 | 성윤우 | input 파일 추가 및 프로젝트 설정 수정, input 기능 수정 |
| 06-08 | 임지섭 | handle_check 브랜치 병합, 충돌 수정 및 프로젝트 설정 |
| 06-09 | 임지섭 | `DirectoryCheck` · `ConsoleColor` 추가, 감시 코드 구현 |
| 06-19 | 성윤우 | CSV 로그 기능 테스트 |
| 06-21 | 성윤우 | fileCheck 기능 수정 (스냅샷 · 통계 · CSV 로그 · 감시 스레드) |
| 11-10 | 임지섭 | feature 브랜치 전체 develop 병합 |

<div align="center">

---

*2026 Yuhan University · Windows Programming*

</div>
