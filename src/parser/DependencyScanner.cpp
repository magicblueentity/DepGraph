#include "DependencyScanner.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QDirIterator>

#include "parser/JSONParser.h"
#include "parser/XMLParser.h"
#include "parser/GradleParser.h"
#include "parser/CMakeParser.h"

static bool readTextFile(const QString& path, QString* out, QString* err)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (err) *err = QString("Cannot open %1").arg(path);
        return false;
    }
    *out = QString::fromUtf8(f.readAll());
    return true;
}

static bool parseRequirementsTxt(const QString& filePath, ParsedDeps* out, QString* err)
{
    out->deps.clear();
    QString txt;
    if (!readTextFile(filePath, &txt, err))
        return false;

    QTextStream ts(&txt);
    while (!ts.atEnd()) {
        QString line = ts.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        // Skip editable installs / includes; still show as unknown dependency
        if (line.startsWith("-r ") || line.startsWith("--requirement"))
            continue;

        // Basic: name==version, name>=version, name
        QRegularExpression re(R"(^([A-Za-z0-9_.-]+)\s*(==|>=|<=|~=|>|<)?\s*([^;\s]+)?)");
        auto m = re.match(line);
        if (!m.hasMatch())
            continue;
        const QString name = m.captured(1);
        const QString op = m.captured(2);
        const QString ver = m.captured(3);
        QString version = ver;
        if (!op.isEmpty() && !ver.isEmpty())
            version = op + ver;
        out->deps.push_back({name, version});
    }

    return true;
}

static QVector<QString> findCandidateFiles(const QDir& repoDir)
{
    QVector<QString> files;
    QDirIterator it(repoDir.absolutePath(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        const QString fn = QFileInfo(path).fileName();

        if (fn.compare("package.json", Qt::CaseInsensitive) == 0 ||
            fn.compare("requirements.txt", Qt::CaseInsensitive) == 0 ||
            fn.compare("pom.xml", Qt::CaseInsensitive) == 0 ||
            fn.compare("build.gradle", Qt::CaseInsensitive) == 0 ||
            fn.compare("build.gradle.kts", Qt::CaseInsensitive) == 0 ||
            fn.compare("CMakeLists.txt", Qt::CaseInsensitive) == 0) {

            // Ignore vendored/build directories to keep scans responsive.
            const QString lower = path.toLower();
            if (lower.contains("/node_modules/") || lower.contains("\\node_modules\\") ||
                lower.contains("/build/") || lower.contains("\\build\\") ||
                lower.contains("/.git/") || lower.contains("\\.git\\") ||
                lower.contains("/dist/") || lower.contains("\\dist\\") ||
                lower.contains("/out/") || lower.contains("\\out\\")) {
                continue;
            }

            files.push_back(path);
        }
    }
    return files;
}

bool DependencyScanner::scanRepositoryToGraph(const QDir& repoDir, GraphModel* graph, QString* err)
{
    if (err) *err = QString();
    if (!repoDir.exists()) {
        if (err) *err = "Repo directory does not exist.";
        return false;
    }

    graph->clear();

    const QString repoName = QFileInfo(repoDir.absolutePath()).fileName();
    const int rootId = graph->upsertNode(repoName.isEmpty() ? "repo" : repoName, "", "repo");

    const QVector<QString> candidates = findCandidateFiles(repoDir);

    // Aggregate: connect root -> each dependency; and connect file-specific pseudo nodes for cross-language mixes.
    for (const QString& filePath : candidates) {
        ParsedDeps parsed;
        QString perr;
        QString fn = QFileInfo(filePath).fileName().toLower();

        QString kind;
        bool ok = false;

        if (fn == "package.json") {
            kind = "npm";
            ok = JSONParser::parsePackageJson(filePath, &parsed, &perr);
        } else if (fn == "requirements.txt") {
            kind = "pypi";
            ok = parseRequirementsTxt(filePath, &parsed, &perr);
        } else if (fn == "pom.xml") {
            kind = "maven";
            ok = XMLParser::parsePomXml(filePath, &parsed, &perr);
        } else if (fn == "build.gradle" || fn == "build.gradle.kts") {
            kind = "gradle";
            ok = GradleParser::parseBuildGradle(filePath, &parsed, &perr);
        } else if (fn == "cmakelists.txt") {
            kind = "cmake";
            ok = CMakeParser::parseCMakeLists(filePath, &parsed, &perr);
        }

        if (!ok) {
            // Non-fatal: skip file but keep scanning.
            continue;
        }

        // Pseudo module node for each file to keep mixes readable
        QString rel = repoDir.relativeFilePath(filePath);
        int moduleId = graph->upsertNode(rel, "", kind + ":module");
        graph->addEdge(rootId, moduleId);

        for (const auto& dep : parsed.deps) {
            const QString name = dep.first;
            const QString version = dep.second;
            int depId = graph->upsertNode(name, version, kind);
            graph->addEdge(moduleId, depId);
        }
    }

    emit graph->changed();
    return true;
}
