// ----------------- mainwindow.h -----------------
#pragma once
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QtWidgets>
#include <opencv2/opencv.hpp>
#include <memory>
#include "node_framework.h"

class PortItem;
class EdgeItem;

class NodeItem : public QGraphicsItem {
public:
    NodeItem(Node* backend, QColor color=Qt::gray);
    QRectF boundingRect() const override;
    void paint(QPainter*,const QStyleOptionGraphicsItem*,QWidget*) override;

    Node* backendNode;
    std::vector<PortItem*> inputs, outputs;
private:
    QColor color; int w=150,h=100;
};

class PortItem : public QGraphicsEllipseItem {
public:
    enum PortType{In,Out};

    PortItem(PortType t,NodeItem* parentNode,int idx);
    void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;

    PortType type; NodeItem* parentNode; int portIndex;
    EdgeItem* tempEdge = nullptr;
};

class EdgeItem : public QGraphicsPathItem {
public:
    EdgeItem(PortItem* from);
    void setTarget(PortItem* to);
    void updatePath(const QPointF& to);
    PortItem *fromPort=nullptr,*toPort=nullptr;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent=nullptr);
    NodeGraph graph;
    void processGraph();
private:
    void setupUI(), setupMenu(), setupBCControls(), setupBlurControls();
    void updateKernelPreview();
    QGraphicsScene* scene;
    QLabel* previewLabel;
    std::shared_ptr<InputNode> inputNodePtr;
    std::shared_ptr<OutputNode> outputNodePtr;
    std::shared_ptr<BrightnessContrastNode> bcNodePtr;
    std::shared_ptr<BlurNode> blurNodePtr;
    // BC UI
    QWidget* bcWidget=nullptr; QSlider *bSlider,*cSlider; QPushButton *rbBtn,*rcBtn;
    // Blur UI
    QWidget* blurWidget=nullptr; QSlider *rSlider,*aSlider,*amtSlider; QComboBox* modeCombo;
    QTableWidget* kernelTable=nullptr;
};
