#pragma once

#include <QObject>
#include <QString>
#include <QDir>

class GitHandler : public QObject {
    Q_OBJECT
public:
    explicit GitHandler(QObject* parent = nullptr);

    // Clones URL into baseDir/<repo-name>-<timestamp>
    // Returns the clone path on success, empty on failure.
    QString cloneRepo(const QString& url, const QDir& baseDir, QString* errorOut);

    static QString guessRepoFolderName(const QString& url);
};
