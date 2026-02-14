#pragma once

#include <QGraphicsView>
#include <QGraphicsItem>
#include <QTimer>
#include <QHash>

#include "model/GraphModel.h"

class GraphView : public QGraphicsView {
    Q_OBJECT
public:
    explicit GraphView(QWidget* parent = nullptr);

    void setModel(GraphModel* model);

    void exportPng(const QString& filePath);
    void exportSvg(const QString& filePath);

signals:
    void nodeSelected(int nodeId);

public slots:
    void focusNode(int nodeId);
    void highlightImpactFrom(int nodeId);
    void clearHighlight();

protected:
    void wheelEvent(QWheelEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private slots:
    void rebuildScene();
    void tickLayout();

private:
    class NodeItem;
    class EdgeItem;

    void fitInitial();
    QColor colorForStatus(NodeStatus s) const;

    GraphModel* m_model = nullptr;
    QGraphicsScene* m_scene = nullptr;

    QTimer m_layoutTimer;

    QHash<int, NodeItem*> m_nodeItems;
    QVector<EdgeItem*> m_edgeItems;

    bool m_panning = false;
    QPoint m_panStart;

    QSet<int> m_highlighted;
};
