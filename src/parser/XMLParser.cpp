#include "XMLParser.h"

#include <tinyxml2.h>

using namespace tinyxml2;

static QString txtOrEmpty(const XMLElement* el)
{
    if (!el || !el->GetText())
        return {};
    return QString::fromUtf8(el->GetText()).trimmed();
}

bool XMLParser::parsePomXml(const QString& filePath, ParsedDeps* out, QString* err)
{
    out->deps.clear();

    XMLDocument doc;
    XMLError rc = doc.LoadFile(filePath.toUtf8().constData());
    if (rc != XML_SUCCESS) {
        if (err) *err = QString("XML load error (%1)").arg(int(rc));
        return false;
    }

    const XMLElement* project = doc.FirstChildElement("project");
    if (!project) {
        if (err) *err = "Not a pom.xml (no <project>)";
        return false;
    }

    const XMLElement* deps = project->FirstChildElement("dependencies");
    if (!deps)
        return true;

    for (const XMLElement* dep = deps->FirstChildElement("dependency"); dep; dep = dep->NextSiblingElement("dependency")) {
        const XMLElement* groupId = dep->FirstChildElement("groupId");
        const XMLElement* artifactId = dep->FirstChildElement("artifactId");
        const XMLElement* version = dep->FirstChildElement("version");

        QString g = txtOrEmpty(groupId);
        QString a = txtOrEmpty(artifactId);
        QString v = txtOrEmpty(version);

        if (a.isEmpty())
            continue;

        QString name = g.isEmpty() ? a : (g + ":" + a);
        out->deps.push_back({name, v});
    }

    return true;
}
