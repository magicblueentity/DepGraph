DepGraph
=======

Qt6 desktop app that scans a repository folder for dependency manifests and renders a dependency graph I made in like 3 hours so don't expect it to be perfect.

Supported inputs (best-effort parsing)
- `package.json` (npm)
- `requirements.txt` (pip)
- `pom.xml` (Maven)
- `build.gradle` / `build.gradle.kts` (Gradle)
- `CMakeLists.txt` (CMake: `find_package`, `FetchContent_Declare`)

Features
- Open a local folder and scan dependencies
- Clone a GitHub repo (requires `git` on PATH) and scan
- Interactive graph view with pan/zoom, node selection, and downstream impact highlighting
- Export graph as JSON/CSV plus PNG/SVG snapshots

Build (CMake)
```powershell
cmake -S . -B build
cmake --build build --config Release
```

Run
- Windows (MSVC): run `build\Release\DepGraph.exe` (or `build\Debug\DepGraph.exe`)
- Windows (Qt MinGW kit): you must have Qt runtime DLLs on PATH, or deploy them next to the exe:

```powershell
# Run from build tree (MinGW)
$env:Path = "C:\Qt\6.10.2\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;$env:Path"
.\build-mingw\DepGraph.exe

# Deploy (copies Qt DLLs + plugins next to DepGraph.exe)
C:\Qt\6.10.2\mingw_64\bin\windeployqt.exe .\build-mingw\DepGraph.exe
```
