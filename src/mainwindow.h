#pragma once
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QtWidgets>
#include <opencv2/opencv.hpp>
#include <memory>
#include "node_framework.h"

class NodeItem : public QGraphicsItem {
public:
    NodeItem(QString title, QColor color = Qt::gray, int id = -1)
        : title(title), color(color), width(150), height(80), nodeId(id) {
        setFlags(QGraphicsItem::ItemIsMovable |
                 QGraphicsItem::ItemSendsGeometryChanges);
    }

    QRectF boundingRect() const override {
        return QRectF(0, 0, width, height);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        painter->setBrush(color);
        painter->drawRoundedRect(0, 0, width, height, 5, 5);
        painter->setPen(Qt::white);
        painter->drawText(10, 20, title);
        painter->setBrush(Qt::darkGray);
        painter->drawEllipse(-5, height/2 - 5, 10, 10);
        painter->drawEllipse(width-5, height/2 - 5, 10, 10);
    }

    int getId() const { return nodeId; }

private:
    QString title;
    QColor color;
    int width, height, nodeId;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void addInputNode();
    void addOutputNode();
    void processGraph();

private:
    void setupUI();
    void setupMenu();

    QGraphicsScene* scene;
    QLabel* previewLabel;

    // GUI items
    NodeItem* inputItem   = nullptr;
    NodeItem* outputItem  = nullptr;

    // Backend nodes
    std::shared_ptr<InputNode>  inputNodePtr;
    std::shared_ptr<OutputNode> outputNodePtr;
    NodeGraph graph;

    int nodeCounter = 0;
};
