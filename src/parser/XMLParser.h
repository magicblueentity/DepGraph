#pragma once

#include "parser/DependencyScanner.h"

class XMLParser {
public:
    static bool parsePomXml(const QString& filePath, ParsedDeps* out, QString* err);
};
