#include "GradleParser.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

static bool readText(const QString& path, QString* out, QString* err)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (err) *err = QString("Cannot open %1").arg(path);
        return false;
    }
    *out = QString::fromUtf8(f.readAll());
    return true;
}

bool GradleParser::parseBuildGradle(const QString& filePath, ParsedDeps* out, QString* err)
{
    out->deps.clear();

    QString txt;
    if (!readText(filePath, &txt, err))
        return false;

    // Extremely pragmatic parsing: capture strings like "group:artifact:version" in dependencies blocks.
    // Works for most common Gradle declarations.
    QRegularExpression re("['\"]([A-Za-z0-9_.-]+):([A-Za-z0-9_.-]+):([^'\"]+)['\"]");
    auto it = re.globalMatch(txt);
    while (it.hasNext()) {
        auto m = it.next();
        QString g = m.captured(1);
        QString a = m.captured(2);
        QString v = m.captured(3);
        out->deps.push_back({g + ":" + a, v});
    }

    // Kotlin DSL variant: implementation("group:artifact:version") is already covered.
    return true;
}
