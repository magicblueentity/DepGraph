#pragma once

#include "parser/DependencyScanner.h"

class GradleParser {
public:
    static bool parseBuildGradle(const QString& filePath, ParsedDeps* out, QString* err);
};
