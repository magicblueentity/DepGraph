#pragma once

#include "parser/DependencyScanner.h"

class CMakeParser {
public:
    static bool parseCMakeLists(const QString& filePath, ParsedDeps* out, QString* err);
};
