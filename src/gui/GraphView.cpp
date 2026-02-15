#include "GraphView.h"

#include <QGraphicsScene>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QRandomGenerator>
#include <QScrollBar>
#include <QSvgGenerator>
#include <QQueue>
#include <QSet>
#include <QtMath>

class GraphView::NodeItem : public QGraphicsObject {
    Q_OBJECT
public:
    NodeItem(const Node& n, GraphView* owner)
        : m_node(n), m_owner(owner)
    {
        setFlags(ItemIsMovable | ItemSendsGeometryChanges | ItemIsSelectable);
        setAcceptHoverEvents(true);
        // Nodes move a lot during layout; caching tends to look "laggy"/smeary.
        setCacheMode(NoCache);

        // deterministic initial scatter (qrand/qsrand were removed in Qt 6)
        QRandomGenerator rng(quint32(n.id * 2654435761u));
        qreal x = (rng.bounded(400)) - 200;
        qreal y = (rng.bounded(260)) - 130;
        setPos(x, y);
    }

    QRectF boundingRect() const override { return QRectF(-m_w/2, -m_h/2, m_w, m_h); }

    void setStatus(NodeStatus s) { m_node.status = s; update(); }
    void setText(const QString& name, const QString& version) { m_node.name = name; m_node.version = version; update(); }

    int nodeId() const { return m_node.id; }
    const Node& node() const { return m_node; }

    QPointF velocity;

    void setHighlighted(bool on) { m_highlight = on; update(); }

signals:
    void clicked(int nodeId);

protected:
    void paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override
    {
        p->setRenderHint(QPainter::Antialiasing, true);

        QColor base = m_owner->colorForStatus(m_node.status);
        QColor fill = base;
        fill.setAlpha(210);

        QColor stroke = QColor(170, 210, 255, 60);
        if (isSelected()) stroke = QColor(255, 255, 255, 140);
        if (m_highlight) stroke = QColor(255, 245, 170, 200);

        QLinearGradient g(boundingRect().topLeft(), boundingRect().bottomRight());
        g.setColorAt(0.0, fill.lighter(120));
        g.setColorAt(1.0, fill.darker(130));

        p->setPen(QPen(stroke, m_highlight ? 2.5 : 1.4));
        p->setBrush(g);
        p->drawRoundedRect(boundingRect(), 16, 16);

        // label
        p->setPen(QColor(230, 240, 255, 235));
        QFont f = p->font();
        f.setBold(true);
        f.setPointSizeF(f.pointSizeF() + 0.5);
        p->setFont(f);

        QString title = m_node.name;
        if (title.size() > 26)
            title = title.left(24) + "..";

        QRectF r = boundingRect().adjusted(12, 10, -12, -10);
        p->drawText(r, Qt::AlignTop | Qt::AlignLeft, title);

        p->setPen(QColor(205, 220, 240, 200));
        QFont f2 = p->font();
        f2.setBold(false);
        f2.setPointSizeF(f2.pointSizeF() - 1);
        p->setFont(f2);

        const QString sub = (m_node.version.isEmpty() ? QString("(") + m_node.kind + ")" : (m_node.version + "  (" + m_node.kind + ")"));
        p->drawText(r, Qt::AlignBottom | Qt::AlignLeft, sub);
    }

    void hoverEnterEvent(QGraphicsSceneHoverEvent*) override
    {
        setToolTip(QString("%1\n%2\nstatus: %3")
                       .arg(m_node.name)
                       .arg(m_node.version.isEmpty() ? "(no version)" : m_node.version)
                       .arg(nodeStatusToString(m_node.status)));
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton) {
            emit clicked(m_node.id);
        }
        QGraphicsObject::mousePressEvent(e);
    }

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override
    {
        if (change == ItemPositionHasChanged) {
            if (m_owner)
                m_owner->updateEdges();
        }
        return QGraphicsObject::itemChange(change, value);
    }

private:
    Node m_node;
    GraphView* m_owner = nullptr;
    qreal m_w = 210;
    qreal m_h = 64;
    bool m_highlight = false;
};

class GraphView::EdgeItem : public QGraphicsItem {
public:
    EdgeItem(NodeItem* a, NodeItem* b)
        : m_a(a), m_b(b)
    {
        setZValue(-10);
        setCacheMode(NoCache);
    }

    QRectF boundingRect() const override
    {
        return m_cachedRect;
    }

    void paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override
    {
        p->setRenderHint(QPainter::Antialiasing, true);
        QPen pen(QColor(120, 170, 255, 60), 1.4);
        p->setPen(pen);
        p->drawPath(m_cachedPath);

        // arrow head near target
        QPointF t = m_cachedPath.pointAtPercent(0.93);
        QPointF t2 = m_cachedPath.pointAtPercent(0.96);
        QPointF dir = (t2 - t);
        if (!dir.isNull()) {
            dir /= std::sqrt(QPointF::dotProduct(dir, dir));
            QPointF left(-dir.y(), dir.x());
            QPointF a1 = t + (-dir * 10) + (left * 4);
            QPointF a2 = t + (-dir * 10) - (left * 4);
            QPolygonF tri;
            tri << t << a1 << a2;
            p->setBrush(QColor(160, 200, 255, 80));
            p->setPen(Qt::NoPen);
            p->drawPolygon(tri);
        }
    }

    void syncGeometry()
    {
        if (!m_a || !m_b)
            return;

        const QPointF p1 = m_a->pos();
        const QPointF p2 = m_b->pos();

        // Curved edge
        QPainterPath path;
        QPointF mid = (p1 + p2) * 0.5;
        QPointF d = p2 - p1;
        QPointF n(-d.y(), d.x());
        const qreal nlen = std::sqrt(QPointF::dotProduct(n, n));
        if (nlen > 1e-6)
            n /= nlen;

        const qreal dist = std::sqrt(QPointF::dotProduct(d, d));
        const qreal bend = qBound(-60.0, dist * 0.10, 60.0);
        const QPointF c = mid + n * bend;
        path.moveTo(p1);
        path.quadTo(c, p2);

        const QRectF nr = path.boundingRect().adjusted(-45, -45, 45, 45);
        if (nr != m_cachedRect) {
            prepareGeometryChange();
            m_cachedRect = nr;
        }
        m_cachedPath = path;
    }

private:
    NodeItem* m_a;
    NodeItem* m_b;
    QPainterPath m_cachedPath;
    QRectF m_cachedRect;
};

GraphView::GraphView(QWidget* parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::NoDrag);
    setOptimizationFlags(QGraphicsView::DontSavePainterState | QGraphicsView::DontAdjustForAntialiasing);

    setBackgroundBrush(QBrush(QColor(8, 12, 18)));
}

QColor GraphView::colorForStatus(NodeStatus s) const
{
    switch (s) {
    case NodeStatus::Stable: return QColor(60, 220, 160);
    case NodeStatus::Outdated: return QColor(245, 200, 80);
    case NodeStatus::Deprecated: return QColor(255, 110, 110);
    case NodeStatus::Conflict: return QColor(255, 80, 160);
    }
    return QColor(60, 220, 160);
}

void GraphView::setModel(GraphModel* model)
{
    if (m_model == model)
        return;

    if (m_model)
        disconnect(m_model, nullptr, this, nullptr);

    m_model = model;
    if (m_model)
        connect(m_model, &GraphModel::changed, this, &GraphView::rebuildScene);

    rebuildScene();
}

void GraphView::rebuildScene()
{
    m_scene->clear();
    m_nodeItems.clear();
    m_edgeItems.clear();
    m_highlighted.clear();

    if (!m_model)
        return;

    // Create nodes
    for (const Node& n : m_model->nodes()) {
        auto* item = new NodeItem(n, this);
        item->setZValue(10);
        connect(item, &NodeItem::clicked, this, &GraphView::nodeSelected);
        m_scene->addItem(item);
        m_nodeItems.insert(n.id, item);
    }

    // Create edges
    for (const Edge& e : m_model->edges()) {
        auto* a = m_nodeItems.value(e.from, nullptr);
        auto* b = m_nodeItems.value(e.to, nullptr);
        if (!a || !b)
            continue;
        auto* edge = new EdgeItem(a, b);
        m_scene->addItem(edge);
        m_edgeItems.push_back(edge);
    }

    applyInitialLayout();
    updateEdges();
    fitInitial();
}

void GraphView::applyInitialLayout()
{
    if (!m_model || m_nodeItems.isEmpty())
        return;

    // Deterministic layered layout from the repo root (node 0). This is fast and stable.
    const int rootId = 0;

    QHash<int, int> depth;
    QQueue<int> q;
    depth.insert(rootId, 0);
    q.enqueue(rootId);

    while (!q.isEmpty()) {
        int cur = q.dequeue();
        const int d = depth.value(cur);
        for (int to : m_model->outgoing(cur)) {
            if (depth.contains(to))
                continue;
            depth.insert(to, d + 1);
            q.enqueue(to);
        }
    }

    // Group nodes by depth. Unreachable nodes go to the last column.
    int maxDepth = 0;
    for (auto it = depth.begin(); it != depth.end(); ++it)
        maxDepth = qMax(maxDepth, it.value());

    QVector<QVector<int>> cols(maxDepth + 2);
    const int unreachableCol = maxDepth + 1;

    for (const Node& n : m_model->nodes()) {
        const int d = depth.contains(n.id) ? depth.value(n.id) : unreachableCol;
        cols[d].push_back(n.id);
    }

    const qreal xStep = 360.0;
    const qreal yStep = 92.0;

    for (int d = 0; d < cols.size(); d++) {
        auto& v = cols[d];
        std::sort(v.begin(), v.end());
        const qreal x = (d - 0) * xStep;
        const qreal y0 = -0.5 * (qMax(1, v.size()) - 1) * yStep;

        for (int i = 0; i < v.size(); i++) {
            if (auto* item = m_nodeItems.value(v[i], nullptr)) {
                item->setPos(QPointF(x, y0 + i * yStep));
            }
        }
    }
}

void GraphView::fitInitial()
{
    QRectF r;
    for (auto* ni : m_nodeItems)
        r |= ni->sceneBoundingRect();
    if (r.isNull()) r = QRectF(-200, -150, 400, 300);
    r.adjust(-120, -120, 120, 120);
    m_scene->setSceneRect(r);
    fitInView(r, Qt::KeepAspectRatio);
    m_zoom = 1.0;
}

void GraphView::wheelEvent(QWheelEvent* e)
{
    const qreal factor = std::pow(1.0012, e->angleDelta().y());
    const qreal next = m_zoom * factor;
    if (next < 0.12 || next > 5.0) {
        e->accept();
        return;
    }
    m_zoom = next;
    scale(factor, factor);
}

void GraphView::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::MiddleButton || (e->button() == Qt::LeftButton && (e->modifiers() & Qt::AltModifier))) {
        m_panning = true;
        m_panStart = e->pos();
        setCursor(Qt::ClosedHandCursor);
        e->accept();
        return;
    }
    QGraphicsView::mousePressEvent(e);
}

void GraphView::mouseMoveEvent(QMouseEvent* e)
{
    if (m_panning) {
        QPoint delta = e->pos() - m_panStart;
        m_panStart = e->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        e->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(e);
}

void GraphView::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_panning && (e->button() == Qt::MiddleButton || e->button() == Qt::LeftButton)) {
        m_panning = false;
        unsetCursor();
        e->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(e);
}

void GraphView::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_F) {
        fitToContents();
        e->accept();
        return;
    }
    QGraphicsView::keyPressEvent(e);
}

void GraphView::focusNode(int nodeId)
{
    auto* ni = m_nodeItems.value(nodeId, nullptr);
    if (!ni)
        return;
    centerOn(ni);
    ni->setSelected(true);
}

void GraphView::fitToContents()
{
    QRectF r = m_scene->itemsBoundingRect();
    if (r.isNull())
        return;
    r.adjust(-120, -120, 120, 120);
    m_scene->setSceneRect(r);
    fitInView(r, Qt::KeepAspectRatio);
    m_zoom = 1.0;
}

void GraphView::resetView()
{
    resetTransform();
    m_zoom = 1.0;
    fitToContents();
}

void GraphView::relayout()
{
    applyInitialLayout();
    updateEdges();
    fitToContents();
}

void GraphView::highlightImpactFrom(int nodeId)
{
    m_highlighted.clear();

    // BFS downstream from nodeId in model edges
    if (!m_model)
        return;

    QQueue<int> q;
    QSet<int> seen;
    q.enqueue(nodeId);
    seen.insert(nodeId);

    while (!q.isEmpty()) {
        int cur = q.dequeue();
        m_highlighted.insert(cur);
        for (int to : m_model->outgoing(cur)) {
            if (!seen.contains(to)) {
                seen.insert(to);
                q.enqueue(to);
            }
        }
    }

    for (auto it = m_nodeItems.begin(); it != m_nodeItems.end(); ++it)
        it.value()->setHighlighted(m_highlighted.contains(it.key()));
}

void GraphView::clearHighlight()
{
    m_highlighted.clear();
    for (auto it = m_nodeItems.begin(); it != m_nodeItems.end(); ++it)
        it.value()->setHighlighted(false);
}

void GraphView::exportPng(const QString& filePath)
{
    QRectF r = m_scene->itemsBoundingRect().adjusted(-40, -40, 40, 40);
    QImage img(r.size().toSize() * 2, QImage::Format_ARGB32_Premultiplied);
    img.fill(QColor(8, 12, 18));

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.translate(-r.topLeft());
    p.scale(2.0, 2.0);
    m_scene->render(&p);
    p.end();

    img.save(filePath);
}

void GraphView::exportSvg(const QString& filePath)
{
    QRectF r = m_scene->itemsBoundingRect().adjusted(-40, -40, 40, 40);

    QSvgGenerator gen;
    gen.setFileName(filePath);
    gen.setSize(r.size().toSize());
    gen.setViewBox(r);

    QPainter p(&gen);
    p.fillRect(r, QColor(8, 12, 18));
    m_scene->render(&p);
    p.end();
}

void GraphView::updateEdges()
{
    for (auto* e : m_edgeItems)
        e->syncGeometry();
}

#include "GraphView.moc"
