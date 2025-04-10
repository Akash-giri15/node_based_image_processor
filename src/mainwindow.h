// ----------------- mainwindow.h -----------------
#pragma once
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QtWidgets>
#include <opencv2/opencv.hpp>

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
        
        // Ports
        painter->setBrush(Qt::darkGray);
        painter->drawEllipse(-5, height/2 - 5, 10, 10);   // Input
        painter->drawEllipse(width-5, height/2 -5, 10,10);// Output
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
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setupUI();
        setupMenu();
    }

private slots:
    void addInputNode() {
        QString file = QFileDialog::getOpenFileName(this, "Open Image");
        if(!file.isEmpty()) {
            NodeItem* item = new NodeItem("Input", Qt::darkGreen, ++nodeCounter);
            scene->addItem(item);
            inputNodes[item->getId()] = file;
            processGraph();
        }
    }

    void addOutputNode() {
        NodeItem* item = new NodeItem("Output", Qt::darkRed, ++nodeCounter);
        scene->addItem(item);
        processGraph();
    }

    void processGraph() {
        if(inputNodes.empty() || !outputNode) return;

        try {
            // Simple processing pipeline
            cv::Mat image = cv::imread(inputNodes.begin()->second.toStdString());
            if(image.empty()) throw std::runtime_error("Failed to load image");
            
            cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
            QImage qimg(image.data, image.cols, image.rows, 
                       image.step, QImage::Format_RGB888);
            
            previewLabel->setPixmap(QPixmap::fromImage(qimg));
        } 
        catch(const std::exception& e) {
            QMessageBox::critical(this, "Error", e.what());
        }
    }

private:
    void setupUI() {
        QSplitter* splitter = new QSplitter(this);
        
        // Scene setup
        scene = new QGraphicsScene(this);
        QGraphicsView* view = new QGraphicsView(scene);
        splitter->addWidget(view);

        // Preview area
        previewLabel = new QLabel();
        previewLabel->setMinimumSize(400, 300);
        splitter->addWidget(previewLabel);

        setCentralWidget(splitter);
        
        // Toolbar
        QToolBar* toolbar = addToolBar("Nodes");
        toolbar->addAction("Input Node", this, &MainWindow::addInputNode);
        toolbar->addAction("Output Node", this, &MainWindow::addOutputNode);
    }

    void setupMenu() {
        QMenu* fileMenu = menuBar()->addMenu("File");
        fileMenu->addAction("Save Output", [this](){
            QString file = QFileDialog::getSaveFileName(this, "Save Image");
            if(!file.isEmpty()) {
                previewLabel->pixmap().save(file);
            }
        });
    }

    QGraphicsScene* scene;
    QLabel* previewLabel;
    std::unordered_map<int, QString> inputNodes;
    bool outputNode = false;
    int nodeCounter = 0;
};