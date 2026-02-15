#pragma once

#include <QMainWindow>
#include <QDir>
#include <QFutureWatcher>

class QLabel;
class QListWidget;
class QLineEdit;
class QSplitter;
class QAction;

#include "model/GraphModel.h"
#include "github/GitHandler.h"

class GraphView;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void openLocalFolder();
    void cloneFromGitHub();
    void rescan();

    void exportJson();
    void exportCsv();
    void exportPng();
    void exportSvg();

    void showAbout();

    void onNodeSelected(int nodeId);
    void applyFilter();
    void focusSelectedListItem();

private:
    void buildUi();
    void setRepoDir(const QDir& dir);
    void scanIntoGraph();
    void setBusy(bool busy, const QString& message = QString());
    void repopulateNodeList();
    QString defaultExportBaseName() const;

    QDir m_repoDir;

    GraphModel m_graph;
    GitHandler m_git;

    GraphView* m_view = nullptr;
    QListWidget* m_nodeList = nullptr;
    QLineEdit* m_filterEdit = nullptr;
    QLabel* m_status = nullptr;

    bool m_updatingListSelection = false;

    QAction* m_actOpen = nullptr;
    QAction* m_actClone = nullptr;
    QAction* m_actRescan = nullptr;
    QAction* m_actJson = nullptr;
    QAction* m_actCsv = nullptr;
    QAction* m_actPng = nullptr;
    QAction* m_actSvg = nullptr;

    QFutureWatcher<GraphModel::Data> m_scanWatcher;
};
