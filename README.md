DepGraph
=======

Qt6 desktop app that scans a repository folder for dependency manifests and renders a dependency graph.

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
- Windows: run `build\Release\DepGraph.exe` (or `build\Debug\DepGraph.exe`)
