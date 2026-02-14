#pragma once

#include <QMainWindow>
#include <QDir>

class QLabel;
class QListWidget;
class QLineEdit;
class QSplitter;

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

    void onNodeSelected(int nodeId);
    void applyFilter();
    void focusSelectedListItem();

private:
    void buildUi();
    void setRepoDir(const QDir& dir);
    void scanIntoGraph();
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
};

