#pragma once

#include <QString>
#include <QVector>
#include <QHash>

enum class NodeStatus {
    Stable,
    Outdated,
    Deprecated,
    Conflict
};

struct Node {
    int id = -1;
    QString name;
    QString version;
    NodeStatus status = NodeStatus::Stable;

    // language/ecosystem hint (npm/pypi/maven/gradle/cmake)
    QString kind;
};

static inline QString nodeStatusToString(NodeStatus s)
{
    switch (s) {
    case NodeStatus::Stable: return "stable";
    case NodeStatus::Outdated: return "outdated";
    case NodeStatus::Deprecated: return "deprecated";
    case NodeStatus::Conflict: return "conflict";
    }
    return "stable";
}
