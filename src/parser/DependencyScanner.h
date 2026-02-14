#pragma once

#include <QString>
#include <QVector>
#include <QPair>
#include <QDir>

#include "model/GraphModel.h"

struct ParsedDeps {
    // pairs: (name, version)
    QVector<QPair<QString, QString>> deps;
};

class JSONParser {
public:
    static bool parsePackageJson(const QString& filePath, ParsedDeps* out, QString* err);
};

class XMLParser {
public:
    static bool parsePomXml(const QString& filePath, ParsedDeps* out, QString* err);
};

class GradleParser {
public:
    static bool parseBuildGradle(const QString& filePath, ParsedDeps* out, QString* err);
};

class CMakeParser {
public:
    static bool parseCMakeLists(const QString& filePath, ParsedDeps* out, QString* err);
};

class DependencyScanner {
public:
    // Scans repo tree for supported files and merges results into the graph.
    // Adds a synthetic root node for the repo.
    static bool scanRepositoryToGraph(const QDir& repoDir, GraphModel* graph, QString* err);
};
