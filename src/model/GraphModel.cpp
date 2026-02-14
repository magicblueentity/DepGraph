#include "GraphModel.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

GraphModel::GraphModel(QObject* parent) : QObject(parent) {}

void GraphModel::clear()
{
    m_nodes.clear();
    m_edges.clear();
    m_keyToId.clear();
    m_out.clear();
    m_in.clear();
    emit changed();
}

int GraphModel::ensureNodeId(const QString& name, const QString& kind)
{
    const QString key = kind + ":" + name.trimmed();
    auto it = m_keyToId.find(key);
    if (it != m_keyToId.end())
        return it.value();

    Node n;
    n.id = m_nodes.size();
    n.name = name.trimmed();
    n.kind = kind.trimmed();
    m_nodes.push_back(n);
    m_keyToId.insert(key, n.id);
    return n.id;
}

int GraphModel::upsertNode(const QString& name, const QString& version, const QString& kind)
{
    int id = ensureNodeId(name, kind);
    Node& n = m_nodes[id];
    if (!version.trimmed().isEmpty())
        n.version = version.trimmed();

    // Heuristics for status: very rough but functional.
    const QString v = n.version;
    if (v.contains("SNAPSHOT", Qt::CaseInsensitive) || v.contains("-alpha", Qt::CaseInsensitive) || v.contains("-beta", Qt::CaseInsensitive))
        n.status = NodeStatus::Outdated;
    if (v.contains("deprecated", Qt::CaseInsensitive))
        n.status = NodeStatus::Deprecated;
    if (v.contains("!"))
        n.status = NodeStatus::Conflict;

    emit changed();
    return id;
}

void GraphModel::addEdge(int fromId, int toId)
{
    if (fromId < 0 || toId < 0 || fromId == toId)
        return;

    if (m_out[fromId].contains(toId))
        return;

    m_edges.push_back({fromId, toId});
    m_out[fromId].insert(toId);
    m_in[toId].insert(fromId);
    emit changed();
}

const Node* GraphModel::nodeById(int id) const
{
    if (id < 0 || id >= m_nodes.size())
        return nullptr;
    return &m_nodes[id];
}

Node* GraphModel::nodeById(int id)
{
    if (id < 0 || id >= m_nodes.size())
        return nullptr;
    return &m_nodes[id];
}

QVector<int> GraphModel::outgoing(int fromId) const
{
    QVector<int> out;
    auto it = m_out.find(fromId);
    if (it == m_out.end())
        return out;
    out.reserve(it.value().size());
    for (int to : it.value())
        out.push_back(to);
    return out;
}

QVector<int> GraphModel::incoming(int toId) const
{
    QVector<int> in;
    auto it = m_in.find(toId);
    if (it == m_in.end())
        return in;
    in.reserve(it.value().size());
    for (int from : it.value())
        in.push_back(from);
    return in;
}

void GraphModel::setNodeVersion(int id, const QString& version)
{
    Node* n = nodeById(id);
    if (!n)
        return;
    n->version = version.trimmed();
    emit changed();
}

void GraphModel::setNodeStatus(int id, NodeStatus status)
{
    Node* n = nodeById(id);
    if (!n)
        return;
    n->status = status;
    emit changed();
}

bool GraphModel::removeEdge(int fromId, int toId)
{
    if (!m_out.contains(fromId) || !m_out[fromId].contains(toId))
        return false;

    m_out[fromId].remove(toId);
    m_in[toId].remove(fromId);

    for (int i = 0; i < m_edges.size(); i++) {
        if (m_edges[i].from == fromId && m_edges[i].to == toId) {
            m_edges.removeAt(i);
            emit changed();
            return true;
        }
    }

    emit changed();
    return true;
}

QByteArray GraphModel::toJson() const
{
    QJsonObject root;
    QJsonArray nodes;
    QJsonArray edges;

    for (const Node& n : m_nodes) {
        QJsonObject o;
        o["id"] = n.id;
        o["name"] = n.name;
        o["version"] = n.version;
        o["status"] = nodeStatusToString(n.status);
        o["kind"] = n.kind;
        nodes.push_back(o);
    }

    for (const Edge& e : m_edges) {
        QJsonObject o;
        o["from"] = e.from;
        o["to"] = e.to;
        edges.push_back(o);
    }

    root["nodes"] = nodes;
    root["edges"] = edges;

    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

QByteArray GraphModel::toCsv() const
{
    QByteArray out;
    out += "type,from,to,id,name,version,status,kind\n";

    for (const Node& n : m_nodes) {
        out += "node,,,";
        out += QByteArray::number(n.id) + ",";
        out += '"' + n.name.toUtf8().replace('"', "\"") + "\",";
        out += '"' + n.version.toUtf8().replace('"', "\"") + "\",";
        out += nodeStatusToString(n.status).toUtf8() + ",";
        out += '"' + n.kind.toUtf8().replace('"', "\"") + "\"\n";
    }

    for (const Edge& e : m_edges) {
        out += "edge," + QByteArray::number(e.from) + "," + QByteArray::number(e.to) + ",,,,,\n";
    }

    return out;
}
