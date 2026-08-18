// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <map>
#include <QtCore>
#include <QQmlEngine>
#include <QQuickItem>
#include "QmlEngineSingleton.h"

#include "visualization/CVisualizationTree.hpp"
#include <visualization-toolkit/ovvizIWindowManager.h>

#include "BoxObject.h"
#include "WindowManagerNode.h"

namespace OpenViBE {
namespace Designer {

class DesignerScenario;

class WindowManager: public QObject, public VisualizationToolkit::IWindowManager
{
  Q_OBJECT

  //Q_PROPERTY(QQmlListProperty<DisplayItem> displays READ displays NOTIFY displaysChanged)
  Q_PROPERTY(QQmlListProperty<WindowManagerNode> boxes READ boxes NOTIFY boxesChanged)
  Q_PROPERTY(QQmlListProperty<WindowManagerNode> unparentedBoxes READ unparentedBoxes NOTIFY unparentedBoxesChanged)
  Q_PROPERTY(WindowManagerNode* root READ root NOTIFY rootChanged)
  Q_PROPERTY(bool showVisualizationWindow READ showVisualizationWindow NOTIFY showVisualizationWindowChanged)

public:
    WindowManager(const Kernel::IKernelContext& ctx, DesignerScenario* scenario);
    ~WindowManager();

    // Property related methods
    QQmlListProperty<WindowManagerNode> boxes();
    QQmlListProperty<WindowManagerNode> unparentedBoxes();
    WindowManagerNode* root() {
        QQmlEngine::setObjectOwnership(m_root, QQmlEngine::CppOwnership);
        return m_root;
    }

    int boxCount();
    WindowManagerNode* boxAt(int index);
    int unparentedBoxCount();
    WindowManagerNode* unparentedBoxAt(int index);
    static qsizetype boxCount(QQmlListProperty<WindowManagerNode>* list);
    static WindowManagerNode* boxAt(QQmlListProperty<WindowManagerNode>* list, qsizetype index);
    static qsizetype unparentedBoxCount(QQmlListProperty<WindowManagerNode>* list);
    static WindowManagerNode* unparentedBoxAt(QQmlListProperty<WindowManagerNode>* list, qsizetype index);

    bool showVisualizationWindow() { return m_showVisualizationWindow; }


    // QML invokable methods
    Q_INVOKABLE void insertRootBox(const QString& id);
    Q_INVOKABLE void insertBox(const QString& id, const QString& parentID, const QString& position);
    Q_INVOKABLE void removeNode(const QString& id);

    Q_INVOKABLE void requestCloseWindow();

    // C++ "invokable" methods
    void addVisualizationBox(BoxObject* box);
    void removeVisualizationBox(BoxObject* box);

    QQmlEngine* getQmlEngine() override;
    void setVisualizationItem(CIdentifier& boxID, QQuickItem* item) override;
    void clearVisualizationItem(CIdentifier& boxID) override;
    void setVisualizationItemFullScreen(CIdentifier& boxID, bool fullScreen) override;

    void show() override;
    void hide() override;

    //QQmlListProperty<DisplayItem> displays();
signals:
    void boxesChanged();
    void unparentedBoxesChanged();
    void rootChanged();
    void showVisualizationWindowChanged();
    void windowCloseRequested();

private:
    const Kernel::IKernelContext& m_kernelCtx;
    QList<WindowManagerNode*> m_boxNodes; // Available visualization boxes (in the tree or not)
    WindowManagerNode* m_root = nullptr; // Root node of the tree
    CVisualizationTree m_tree;
    CIdentifier m_windowID;  // id of the window in the visualization tree. The child component of the window will be used to create the root node.

    bool m_showVisualizationWindow = false;

    WindowManagerNode::NodeType getTypeFromWidgetType(const VisualizationToolkit::EVisualizationWidget type);
    VisualizationToolkit::EDragLocation getLocationFromString(const QString& position);
    void clearTreeVisualRepresentation();
    void updateTreeVisualRepresentation();
    void populateTreeNodeChildren(WindowManagerNode* node);
    void clearTreeNodeChildren(WindowManagerNode* node);
    bool isNodeInTree(WindowManagerNode* node) const;
};

}  // Designer
}  // OpenViBE