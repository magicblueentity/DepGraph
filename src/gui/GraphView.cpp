#include "GraphView.h"

#include <QGraphicsScene>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QSvgGenerator>
#include <QtMath>

class GraphView::NodeItem : public QGraphicsObject {
    Q_OBJECT
public:
    NodeItem(const Node& n, GraphView* owner)
        : m_node(n), m_owner(owner)
    {
        setFlags(ItemIsMovable | ItemSendsGeometryChanges | ItemIsSelectable);
        setAcceptHoverEvents(true);
        setCacheMode(DeviceCoordinateCache);

        // deterministic initial scatter
        qsrand(uint(n.id * 2654435761u));
        qreal x = (qrand() % 400) - 200;
        qreal y = (qrand() % 260) - 130;
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
            // keep velocity mild when user drags
            velocity *= 0.2;
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
        setCacheMode(DeviceCoordinateCache);
    }

    QRectF boundingRect() const override
    {
        QPointF p1 = m_a->pos();
        QPointF p2 = m_b->pos();
        QRectF r(p1, p2);
        r = r.normalized();
        r.adjust(-40, -40, 40, 40);
        return r;
    }

    void paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override
    {
        p->setRenderHint(QPainter::Antialiasing, true);
        QPointF p1 = m_a->pos();
        QPointF p2 = m_b->pos();

        // Curved edge with subtle glow
        QPainterPath path;
        QPointF mid = (p1 + p2) * 0.5;
        QPointF d = p2 - p1;
        QPointF n(-d.y(), d.x());
        if (!n.isNull()) n /= std::sqrt(QPointF::dotProduct(n, n));
        qreal bend = qBound(-60.0, std::sqrt(QPointF::dotProduct(d, d)) * 0.10, 60.0);
        QPointF c = mid + n * bend;
        path.moveTo(p1);
        path.quadTo(c, p2);

        QPen pen(QColor(120, 170, 255, 60), 1.4);
        p->setPen(pen);
        p->drawPath(path);

        // arrow head near target
        QPointF t = path.pointAtPercent(0.93);
        QPointF t2 = path.pointAtPercent(0.96);
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

private:
    NodeItem* m_a;
    NodeItem* m_b;
};

GraphView::GraphView(QWidget* parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::NoDrag);

    setBackgroundBrush(QBrush(QColor(8, 12, 18)));

    connect(&m_layoutTimer, &QTimer::timeout, this, &GraphView::tickLayout);
    m_layoutTimer.start(16); // ~60fps
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

    fitInitial();
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
}

void GraphView::tickLayout()
{
    if (!m_model || m_nodeItems.isEmpty())
        return;

    // Force-directed layout (basic but smooth):
    // - repulsion between nodes
    // - spring along edges
    // - mild centering

    const qreal repulsion = 22000.0;
    const qreal spring = 0.0048;
    const qreal desired = 160.0;
    const qreal damping = 0.85;
    const qreal maxStep = 12.0;

    QVector<NodeItem*> items;
    items.reserve(m_nodeItems.size());
    for (auto* it : m_nodeItems)
        items.push_back(it);

    // Repulsion
    for (int i = 0; i < items.size(); i++) {
        for (int j = i + 1; j < items.size(); j++) {
            NodeItem* a = items[i];
            NodeItem* b = items[j];
            QPointF d = b->pos() - a->pos();
            qreal dist2 = QPointF::dotProduct(d, d) + 20.0;
            qreal f = repulsion / dist2;
            QPointF dir = d / std::sqrt(dist2);
            a->velocity -= dir * f;
            b->velocity += dir * f;
        }
    }

    // Springs along edges
    for (const Edge& e : m_model->edges()) {
        NodeItem* a = m_nodeItems.value(e.from, nullptr);
        NodeItem* b = m_nodeItems.value(e.to, nullptr);
        if (!a || !b)
            continue;
        QPointF d = b->pos() - a->pos();
        qreal dist = std::sqrt(QPointF::dotProduct(d, d) + 1.0);
        QPointF dir = d / dist;
        qreal stretch = dist - desired;
        QPointF f = dir * (stretch * spring * 1000.0);
        a->velocity += f;
        b->velocity -= f;
    }

    // Centering
    for (auto* a : items) {
        QPointF toCenter = -a->pos();
        a->velocity += toCenter * 0.0009;
    }

    // Integrate
    for (auto* a : items) {
        if (a->isSelected()) {
            // user is likely dragging
            a->velocity *= 0.2;
            continue;
        }

        a->velocity *= damping;
        QPointF step = a->velocity;
        qreal len = std::sqrt(QPointF::dotProduct(step, step));
        if (len > maxStep)
            step = step / len * maxStep;
        a->setPos(a->pos() + step);
    }

    // Expand scene rect gradually to follow nodes
    QRectF r;
    for (auto* ni : m_nodeItems)
        r |= ni->sceneBoundingRect();
    if (!r.isNull()) {
        r.adjust(-160, -160, 160, 160);
        m_scene->setSceneRect(r);
    }

    // Edges are QGraphicsItem; they update via boundingRect recompute
    for (auto* e : m_edgeItems)
        e->update();
}

void GraphView::wheelEvent(QWheelEvent* e)
{
    const qreal factor = std::pow(1.0015, e->angleDelta().y());
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

void GraphView::focusNode(int nodeId)
{
    auto* ni = m_nodeItems.value(nodeId, nullptr);
    if (!ni)
        return;
    centerOn(ni);
    ni->setSelected(true);
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

#include "GraphView.moc"
