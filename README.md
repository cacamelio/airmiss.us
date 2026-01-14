<div align="center">

# ☁️ airmiss.su

![Version](https://img.shields.io/badge/version-1.5--modified-blue?style=flat-square)
![Platform](https://img.shields.io/badge/platform-WINDOWS-lightgrey?style=flat-square)
![Base](https://img.shields.io/badge/based%20on-airflow.su-red?style=flat-square)

**High-performance internal cheat for Counter-Strike: Global Offensive (Legacy).**<br>
*Built on the robust architecture of [airflow.su](https://github.com/HOLOWIFEP/airflow-csgo-cheat) (v1.5).*

[ Installation ] • [ Credits ] • [ License ]

</div>

---

## 🚀 Introduction

**airmiss.su** is a custom internal modification of the airflow.su source code. This project is maintained by a small team dedicated to refining and expanding the codebase for the legacy CS:GO version.

## 👥 Developers

The minds behind the paste:

* **cacamelio** - Developer
* **kiloV2 (Filip)** - Developer
* **cloxy** - The ultimate pasta king

## 🛠️ Building & Usage

To build this project, ensure you have the required SDKs installed.

### Prerequisites
* **IDE:** Visual Studio 2019 or 2022
* **Workload:** Desktop development with C++
* **SDK:** DirectX SDK (June 2010)

### Instructions
1.  Clone the repository:
    ```bash
    git clone [https://github.com/cacamelio/airmiss.us.git](https://github.com/cacamelio/airmiss.us.git)
    ```
2.  Open the `.sln` solution file in Visual Studio.
3.  Change the build configuration to **Release | x86**.
4.  Build the solution (`Ctrl + Shift + B`).
5.  Inject the output `.dll` into `csgo.exe`.

## 📂 Source Overview

The project is structured for modularity:

* `ragebot.cpp` - Rage aimbot logic.
* `anti_aim.cpp` - Desync and angle management.
* `render.cpp` - Visuals and rendering loop.
* `hooks.cpp` - VMT and Detour hooking.
* `imgui/` - Menu interface.

## 📜 Credits & Libraries

* **[HOLOWIFEP](https://github.com/HOLOWIFEP)** - Original creator of airflow.su.
* **ImGui** - GUI Library.
* **MinHook** - Hooking Library.

---

<div align="center">
    <sub><b>Educational Purposes Only.</b> CS:GO is dead, long live CS2.</sub>
</div>
