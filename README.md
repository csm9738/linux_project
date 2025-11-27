# 🔭 GitScope

**터미널에서 쓰는 인터랙티브 Git 트리 시각화 도구 (Interactive CLI Git Tree Visualizer)**

![License](https://img.shields.io/badge/license-MIT-blue.svg) ![Platform](https://img.imgshields.io/badge/platform-Linux-lightgrey) ![Language](https://img.shields.io/badge/language-C%20%7C%20Bash-orange)

> **"VS Code는 너무 무겁고, `git log`는 가독성이 떨어져요. 터미널을 떠나지 않고 그래프를 볼 수는 없을까요?"**
>
> 저희는 터미널(CLI)을 사랑하는 개발자입니다. 복잡한 Git 히스토리를 깔끔하게 볼 수 있도록, **C언어**와 **ncurses** 기반으로 **GitScope**를 만들었습니다.

---

## 🤔 GitScope, 왜 만들었나요? (Motivation)

대부분의 Git Graph 도구는 무거운 IDE나 GUI 앱을 요구합니다. 저희는 이런 비효율을 해결하고 싶었습니다.

GitScope의 핵심 목표는 **가볍고 빠르며, 동시에 인터랙티브한 경험**을 제공하는 것입니다. 이제 `git log`의 복잡한 출력 대신, **GUI 수준의 시각화**를 리눅스 터미널에서 바로 경험해 보세요.

---

## ✨ 주요 기능 (Key Features)

### 🌳 Git Log Tree 시각화
`git log` 명령어 출력을 파싱하여 **가독성이 매우 높은 트리 구조**로 렌더링합니다.
- **깔끔한 분기:** 브랜치 분기(Branching)와 병합(Merging) 지점을 명확하게 표시해 히스토리 파악이 쉽습니다.
- **핵심 정보:** 커밋 해시, 작성자, 메시지를 한눈에 확인할 수 있습니다.

### 🎨 테마 및 디자인 커스터마이징
개인의 취향이나 터미널 환경에 맞춰 UI를 커스터마이징할 수 있습니다.
- **환경변수 설정:** **`export`** 명령어를 통해 테마(Dark/Light/Custom)와 트리 연결선 스타일(ASCII/Unicode)을 손쉽게 변경할 수 있습니다.

### ⚡ 파일 미리보기 (Instant File Preview)
커밋을 탐색하다가 특정 변경 사항이 궁금할 때, 엔터키 하나로 해결하세요.
- 리눅스 네이티브 명령어인 **`cat`**이나 **`head`**를 활용하여 변경된 파일 내용을 터미널 내에서 즉시 확인합니다.

### ✅ 커밋 규칙 추천 가이드
팀의 커밋 컨벤션(예: `feat:`, `fix:`)을 준수할 수 있도록 도와줍니다.
- 인터랙티브 UI가 **규칙에 맞는 접두사**를 추천하여 깔끔한 커밋 메시지를 작성할 수 있도록 돕습니다.

---

## 🛠 아키텍처 및 기술 스택 (Under the Hood)

GitScope는 성능과 유연성을 모두 잡기 위해 **C + Shell Hybrid Architecture**를 사용합니다.

| 역할 | 기술 스택 | 설명 |
| :--- | :--- | :--- |
| **핵심 성능 (Core Logic)** | **C Language & ncurses** | Git Log 파싱, Tree 계산, 터미널 UI 렌더링 등 성능이 핵심인 부분을 담당합니다. |
| **제어 및 통합 (Control Layer)** | **Bash Shell Script** | 전체 실행 흐름 제어, 프로세스 파이프라인 연결, 환경변수 관리를 담당하는 '접착제' 역할입니다. |
| **시스템 통합** | **Linux System Calls** | `fork()`, `exec()`, `pipe()` 등의 시스템 호출을 적극적으로 활용하여 리눅스 환경과 완벽하게 통합됩니다. |

---