#include "JSONParser.h"

#include <QFile>
#include <QString>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

static bool readAllBytes(const QString& path, QByteArray* out, QString* err)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (err) *err = QString("Cannot open %1").arg(path);
        return false;
    }
    *out = f.readAll();
    return true;
}

static void collectDeps(const json& obj, QVector<QPair<QString, QString>>* out)
{
    if (!obj.is_object())
        return;
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        const std::string k = it.key();
        QString name = QString::fromStdString(k);
        QString ver;
        if (it.value().is_string())
            ver = QString::fromStdString(it.value().get<std::string>());
        out->push_back({name, ver});
    }
}

bool JSONParser::parsePackageJson(const QString& filePath, ParsedDeps* out, QString* err)
{
    out->deps.clear();

    QByteArray bytes;
    if (!readAllBytes(filePath, &bytes, err))
        return false;

    json j;
    try {
        j = json::parse(bytes.constData(), bytes.constData() + bytes.size());
    } catch (const std::exception& e) {
        if (err) *err = QString("JSON parse error: %1").arg(e.what());
        return false;
    }

    // Dependencies commonly in: dependencies, devDependencies, peerDependencies, optionalDependencies
    if (j.contains("dependencies")) collectDeps(j["dependencies"], &out->deps);
    if (j.contains("devDependencies")) collectDeps(j["devDependencies"], &out->deps);
    if (j.contains("peerDependencies")) collectDeps(j["peerDependencies"], &out->deps);
    if (j.contains("optionalDependencies")) collectDeps(j["optionalDependencies"], &out->deps);

    return true;
}
