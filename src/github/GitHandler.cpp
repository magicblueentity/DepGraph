#include "GitHandler.h"

#include <QDateTime>
#include <QProcess>
#include <QRegularExpression>

GitHandler::GitHandler(QObject* parent) : QObject(parent) {}

QString GitHandler::guessRepoFolderName(const QString& url)
{
    // Supports: https://github.com/owner/repo(.git) and git@github.com:owner/repo(.git)
    QString s = url.trimmed();
    s.replace("\\", "/");

    QRegularExpression re1("github\\.com[:/](?<owner>[^/]+)/(?<repo>[^/]+)$", QRegularExpression::CaseInsensitiveOption);
    auto m = re1.match(s);
    if (m.hasMatch()) {
        QString repo = m.captured("repo");
        if (repo.endsWith(".git", Qt::CaseInsensitive))
            repo.chop(4);
        return repo;
    }

    // fallback: take last path segment
    int idx = s.lastIndexOf('/');
    QString repo = (idx >= 0) ? s.mid(idx + 1) : s;
    if (repo.endsWith(".git", Qt::CaseInsensitive))
        repo.chop(4);
    return repo.isEmpty() ? "repo" : repo;
}

QString GitHandler::cloneRepo(const QString& url, const QDir& baseDir, QString* errorOut)
{
    if (errorOut) *errorOut = QString();

    if (!baseDir.exists()) {
        if (errorOut) *errorOut = "Base directory does not exist.";
        return {};
    }

    QString folder = guessRepoFolderName(url);
    QString ts = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
    QString targetPath = baseDir.absoluteFilePath(QString("%1-%2").arg(folder, ts));

    QProcess p;
    p.setProgram("git");
    p.setArguments({"clone", "--depth", "1", url, targetPath});
    p.setProcessChannelMode(QProcess::MergedChannels);

    p.start();
    if (!p.waitForStarted(5000)) {
        if (errorOut) *errorOut = "Failed to start git. Is git installed and on PATH?";
        return {};
    }

    if (!p.waitForFinished(-1)) {
        if (errorOut) *errorOut = "git clone did not finish.";
        return {};
    }

    const int code = p.exitCode();
    const QString out = QString::fromUtf8(p.readAllStandardOutput());

    if (code != 0) {
        if (errorOut) *errorOut = QString("git clone failed (exit %1):\n%2").arg(code).arg(out);
        return {};
    }

    return targetPath;
}
