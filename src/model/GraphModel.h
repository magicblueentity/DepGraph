#pragma once

#include <QObject>
#include <QVector>
#include <QHash>
#include <QSet>

#include "model/Node.h"
#include "model/Edge.h"

class GraphModel : public QObject {
    Q_OBJECT
public:
    explicit GraphModel(QObject* parent = nullptr);

    struct Data {
        QVector<Node> nodes;
        QVector<Edge> edges;
        QHash<QString, int> keyToId;
        QHash<int, QSet<int>> out;
        QHash<int, QSet<int>> in;
    };

    void clear();
    // Replace entire graph contents from another instance (used to apply results
    // built in a worker thread onto the GUI-owned model).
    void replaceFrom(const GraphModel& other);
    void replaceFromData(const Data& data);
    Data toData() const;

    int upsertNode(const QString& name, const QString& version, const QString& kind);
    void addEdge(int fromId, int toId);

    const QVector<Node>& nodes() const { return m_nodes; }
    const QVector<Edge>& edges() const { return m_edges; }

    const Node* nodeById(int id) const;
    Node* nodeById(int id);

    QVector<int> outgoing(int fromId) const;
    QVector<int> incoming(int toId) const;

    // simulation helpers
    void setNodeVersion(int id, const QString& version);
    void setNodeStatus(int id, NodeStatus status);
    bool removeEdge(int fromId, int toId);

    // export
    QByteArray toJson() const;
    QByteArray toCsv() const;

signals:
    void changed();

private:
    int ensureNodeId(const QString& name, const QString& kind);

    QVector<Node> m_nodes;
    QVector<Edge> m_edges;
    QHash<QString, int> m_keyToId; // kind + ":" + name

    QHash<int, QSet<int>> m_out;
    QHash<int, QSet<int>> m_in;
};
