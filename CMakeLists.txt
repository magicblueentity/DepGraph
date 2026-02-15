cmake_minimum_required(VERSION 3.20)
project(DepGraph VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt
find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Svg Concurrent)
qt_standard_project_setup()

include(FetchContent)

# nlohmann/json
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# tinyxml2
FetchContent_Declare(
  tinyxml2
  GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
  GIT_TAG 10.0.0
)
FetchContent_MakeAvailable(tinyxml2)

set(APP_SOURCES
  src/main.cpp
  src/gui/MainWindow.h
  src/gui/MainWindow.cpp
  src/gui/GraphView.h
  src/gui/GraphView.cpp
  src/model/GraphModel.h
  src/model/GraphModel.cpp
  src/model/Node.h
  src/model/Node.cpp
  src/model/Edge.h
  src/model/Edge.cpp
  src/github/GitHandler.h
  src/github/GitHandler.cpp
  src/parser/DependencyScanner.h
  src/parser/DependencyScanner.cpp
  src/parser/JSONParser.h
  src/parser/JSONParser.cpp
  src/parser/XMLParser.h
  src/parser/XMLParser.cpp
  src/parser/CMakeParser.h
  src/parser/CMakeParser.cpp
  src/parser/GradleParser.h
  src/parser/GradleParser.cpp
  resources/depgraph.qrc
)

qt_add_executable(DepGraph WIN32 MACOSX_BUNDLE ${APP_SOURCES})

target_include_directories(DepGraph PRIVATE src)

# MinGW: GCC invokes binutils (e.g. `as`) by name, so it must be discoverable.
# Adding `-B <mingw-bin>` makes GCC search that directory for helper programs
# even if PATH is missing/changed between configure/build shells.
if (MINGW)
  get_filename_component(_mingw_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
  target_compile_options(DepGraph PRIVATE "-B${_mingw_bin_dir}/")
  target_link_options(DepGraph PRIVATE "-B${_mingw_bin_dir}/")
endif()

target_link_libraries(DepGraph PRIVATE
  Qt6::Widgets
  Qt6::Svg
  Qt6::Concurrent
  nlohmann_json::nlohmann_json
  tinyxml2
)

# Helpful warnings
if (MSVC)
  target_compile_options(DepGraph PRIVATE /W4 /permissive-)
else()
  target_compile_options(DepGraph PRIVATE -Wall -Wextra -Wpedantic)
endif()
