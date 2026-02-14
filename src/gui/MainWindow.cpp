#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QSaveFile>

#include "gui/GraphView.h"
#include "parser/DependencyScanner.h"

static bool writeBytesAtomically(const QString& path, const QByteArray& bytes, QString* err)
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (err) *err = QString("Cannot write %1").arg(path);
        return false;
    }
    if (f.write(bytes) != bytes.size()) {
        if (err) *err = QString("Failed writing %1").arg(path);
        return false;
    }
    if (!f.commit()) {
        if (err) *err = QString("Failed committing %1").arg(path);
        return false;
    }
    return true;
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_git(this)
{
    buildUi();

    m_view->setModel(&m_graph);
    connect(m_view, &GraphView::nodeSelected, this, &MainWindow::onNodeSelected);
    connect(&m_graph, &GraphModel::changed, this, &MainWindow::repopulateNodeList);

    statusBar()->showMessage("Open a folder or clone a repo to scan dependencies.");
}

void MainWindow::buildUi()
{
    setWindowTitle("DepGraph");

    auto* tb = addToolBar("Main");
    tb->setMovable(false);

    auto* actOpen = new QAction("Open Folder", this);
    connect(actOpen, &QAction::triggered, this, &MainWindow::openLocalFolder);
    tb->addAction(actOpen);

    auto* actClone = new QAction("Clone GitHub", this);
    connect(actClone, &QAction::triggered, this, &MainWindow::cloneFromGitHub);
    tb->addAction(actClone);

    tb->addSeparator();

    auto* actRescan = new QAction("Rescan", this);
    connect(actRescan, &QAction::triggered, this, &MainWindow::rescan);
    tb->addAction(actRescan);

    tb->addSeparator();

    auto* actJson = new QAction("Export JSON", this);
    connect(actJson, &QAction::triggered, this, &MainWindow::exportJson);
    tb->addAction(actJson);

    auto* actCsv = new QAction("Export CSV", this);
    connect(actCsv, &QAction::triggered, this, &MainWindow::exportCsv);
    tb->addAction(actCsv);

    auto* actPng = new QAction("Export PNG", this);
    connect(actPng, &QAction::triggered, this, &MainWindow::exportPng);
    tb->addAction(actPng);

    auto* actSvg = new QAction("Export SVG", this);
    connect(actSvg, &QAction::triggered, this, &MainWindow::exportSvg);
    tb->addAction(actSvg);

    auto* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction(actOpen);
    fileMenu->addAction(actClone);
    fileMenu->addAction(actRescan);
    fileMenu->addSeparator();
    fileMenu->addAction(actJson);
    fileMenu->addAction(actCsv);
    fileMenu->addAction(actPng);
    fileMenu->addAction(actSvg);
    fileMenu->addSeparator();

    auto* actQuit = new QAction("Quit", this);
    connect(actQuit, &QAction::triggered, qApp, &QApplication::quit);
    fileMenu->addAction(actQuit);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* splitter = new QSplitter(Qt::Horizontal, central);

    auto* left = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(10, 10, 10, 10);
    leftLayout->setSpacing(8);

    m_filterEdit = new QLineEdit(left);
    m_filterEdit->setPlaceholderText("Filter nodes (substring)...");
    connect(m_filterEdit, &QLineEdit::textChanged, this, &MainWindow::applyFilter);
    leftLayout->addWidget(m_filterEdit);

    m_nodeList = new QListWidget(left);
    m_nodeList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_nodeList, &QListWidget::itemSelectionChanged, this, &MainWindow::focusSelectedListItem);
    leftLayout->addWidget(m_nodeList, 1);

    m_status = new QLabel(left);
    m_status->setWordWrap(true);
    m_status->setText("No repository loaded.");
    leftLayout->addWidget(m_status);

    m_view = new GraphView(splitter);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({360, 1000});

    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(splitter);
}

void MainWindow::setRepoDir(const QDir& dir)
{
    m_repoDir = dir;
    setWindowTitle(QString("DepGraph  [%1]").arg(m_repoDir.absolutePath()));
}

void MainWindow::openLocalFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select repository folder");
    if (dir.isEmpty())
        return;

    setRepoDir(QDir(dir));
    scanIntoGraph();
}

void MainWindow::cloneFromGitHub()
{
    bool ok = false;
    QString url = QInputDialog::getText(
        this,
        "Clone GitHub Repo",
        "Repo URL (https://github.com/owner/repo or git@github.com:owner/repo):",
        QLineEdit::Normal,
        "https://github.com/",
        &ok);

    if (!ok || url.trimmed().isEmpty())
        return;

    QString base = QFileDialog::getExistingDirectory(this, "Select clone base folder");
    if (base.isEmpty())
        return;

    QString err;
    const QString clonePath = m_git.cloneRepo(url, QDir(base), &err);
    if (clonePath.isEmpty()) {
        QMessageBox::warning(this, "Clone failed", err.isEmpty() ? "Unknown error" : err);
        return;
    }

    setRepoDir(QDir(clonePath));
    scanIntoGraph();
}

void MainWindow::rescan()
{
    if (!m_repoDir.exists()) {
        QMessageBox::information(this, "No repo", "Open a folder or clone a repo first.");
        return;
    }
    scanIntoGraph();
}

void MainWindow::scanIntoGraph()
{
    QString err;
    if (!DependencyScanner::scanRepositoryToGraph(m_repoDir, &m_graph, &err)) {
        QMessageBox::warning(this, "Scan failed", err.isEmpty() ? "Unknown error" : err);
        return;
    }

    const int nNodes = m_graph.nodes().size();
    const int nEdges = m_graph.edges().size();

    m_status->setText(QString("Repo: %1\nNodes: %2   Edges: %3")
                          .arg(m_repoDir.absolutePath())
                          .arg(nNodes)
                          .arg(nEdges));

    statusBar()->showMessage(QString("Scan complete. %1 nodes, %2 edges.").arg(nNodes).arg(nEdges), 3500);
    applyFilter();
}

QString MainWindow::defaultExportBaseName() const
{
    if (m_repoDir.exists())
        return m_repoDir.dirName().isEmpty() ? "depgraph" : m_repoDir.dirName();
    return "depgraph";
}

void MainWindow::exportJson()
{
    const QString path = QFileDialog::getSaveFileName(this, "Export JSON", defaultExportBaseName() + ".json", "JSON (*.json)");
    if (path.isEmpty())
        return;
    QString err;
    if (!writeBytesAtomically(path, m_graph.toJson(), &err))
        QMessageBox::warning(this, "Export failed", err);
}

void MainWindow::exportCsv()
{
    const QString path = QFileDialog::getSaveFileName(this, "Export CSV", defaultExportBaseName() + ".csv", "CSV (*.csv)");
    if (path.isEmpty())
        return;
    QString err;
    if (!writeBytesAtomically(path, m_graph.toCsv(), &err))
        QMessageBox::warning(this, "Export failed", err);
}

void MainWindow::exportPng()
{
    const QString path = QFileDialog::getSaveFileName(this, "Export PNG", defaultExportBaseName() + ".png", "PNG (*.png)");
    if (path.isEmpty())
        return;
    m_view->exportPng(path);
}

void MainWindow::exportSvg()
{
    const QString path = QFileDialog::getSaveFileName(this, "Export SVG", defaultExportBaseName() + ".svg", "SVG (*.svg)");
    if (path.isEmpty())
        return;
    m_view->exportSvg(path);
}

void MainWindow::onNodeSelected(int nodeId)
{
    if (m_updatingListSelection)
        return;

    const Node* n = m_graph.nodeById(nodeId);
    if (!n)
        return;

    // Best-effort: select the matching list row by storing nodeId in item data.
    m_updatingListSelection = true;
    for (int i = 0; i < m_nodeList->count(); i++) {
        auto* it = m_nodeList->item(i);
        if (!it)
            continue;
        if (it->data(Qt::UserRole).toInt() == nodeId) {
            m_nodeList->setCurrentRow(i);
            break;
        }
    }
    m_updatingListSelection = false;

    statusBar()->showMessage(QString("%1  %2  [%3]").arg(n->name, n->version, n->kind), 4000);
}

void MainWindow::repopulateNodeList()
{
    // Keep selection stable if possible; rebuild contents based on filter.
    applyFilter();
}

void MainWindow::applyFilter()
{
    const QString needle = m_filterEdit ? m_filterEdit->text().trimmed() : QString();
    const int keepSelectedId = (m_nodeList && m_nodeList->currentItem())
                                  ? m_nodeList->currentItem()->data(Qt::UserRole).toInt()
                                  : -1;

    m_updatingListSelection = true;
    m_nodeList->clear();

    for (const Node& n : m_graph.nodes()) {
        if (!needle.isEmpty()) {
            if (!n.name.contains(needle, Qt::CaseInsensitive) &&
                !n.kind.contains(needle, Qt::CaseInsensitive) &&
                !n.version.contains(needle, Qt::CaseInsensitive)) {
                continue;
            }
        }

        QString line = n.name;
        if (!n.version.isEmpty())
            line += "  " + n.version;
        if (!n.kind.isEmpty())
            line += "  (" + n.kind + ")";

        auto* item = new QListWidgetItem(line, m_nodeList);
        item->setData(Qt::UserRole, n.id);
        if (n.status == NodeStatus::Outdated)
            item->setForeground(QColor(245, 200, 80));
        else if (n.status == NodeStatus::Deprecated)
            item->setForeground(QColor(255, 110, 110));
        else if (n.status == NodeStatus::Conflict)
            item->setForeground(QColor(255, 80, 160));
    }

    // Restore selection.
    if (keepSelectedId >= 0) {
        for (int i = 0; i < m_nodeList->count(); i++) {
            auto* it = m_nodeList->item(i);
            if (it && it->data(Qt::UserRole).toInt() == keepSelectedId) {
                m_nodeList->setCurrentRow(i);
                break;
            }
        }
    }

    m_updatingListSelection = false;
}

void MainWindow::focusSelectedListItem()
{
    if (m_updatingListSelection)
        return;

    auto* it = m_nodeList->currentItem();
    if (!it)
        return;

    const int nodeId = it->data(Qt::UserRole).toInt();
    m_view->focusNode(nodeId);
    m_view->highlightImpactFrom(nodeId);
}

