#pragma once

#include "parser/DependencyScanner.h"

class JSONParser {
public:
    static bool parsePackageJson(const QString& filePath, ParsedDeps* out, QString* err);
};
