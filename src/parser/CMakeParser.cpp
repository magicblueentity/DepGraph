#include "CMakeParser.h"

#include <QFile>
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

static QString stripComments(const QString& s)
{
    // Remove line comments starting with #
    QStringList lines = s.split('\n');
    for (QString& l : lines) {
        int i = l.indexOf('#');
        if (i >= 0)
            l = l.left(i);
    }
    return lines.join("\n");
}

bool CMakeParser::parseCMakeLists(const QString& filePath, ParsedDeps* out, QString* err)
{
    out->deps.clear();

    QString txt;
    if (!readText(filePath, &txt, err))
        return false;

    txt = stripComments(txt);

    // find_package(Foo ...)
    {
        QRegularExpression re("find_package\\s*\\(\\s*([A-Za-z0-9_+-]+)(?:\\s+([0-9][^\\s\\)]*))?", QRegularExpression::CaseInsensitiveOption);
        auto it = re.globalMatch(txt);
        while (it.hasNext()) {
            auto m = it.next();
            QString name = m.captured(1);
            QString ver = m.captured(2);
            out->deps.push_back({name, ver});
        }
    }

    // FetchContent_Declare(name ... GIT_TAG vX)
    {
        QRegularExpression re("FetchContent_Declare\\s*\\(\\s*([A-Za-z0-9_+-]+)([\\s\\S]*?)\\)", QRegularExpression::CaseInsensitiveOption);
        auto it = re.globalMatch(txt);
        while (it.hasNext()) {
            auto m = it.next();
            QString name = m.captured(1);
            QString body = m.captured(2);
            QString ver;
            QRegularExpression tagRe("GIT_TAG\\s+([^\\s\\)]+)", QRegularExpression::CaseInsensitiveOption);
            auto tm = tagRe.match(body);
            if (tm.hasMatch())
                ver = tm.captured(1);
            out->deps.push_back({name, ver});
        }
    }

    return true;
}
