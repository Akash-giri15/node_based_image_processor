// ----------------- mainwindow.cpp -----------------
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUI();
    setupMenu();
}

void MainWindow::setupUI() {
    QSplitter* splitter = new QSplitter(this);
    scene = new QGraphicsScene(this);
    QGraphicsView* view = new QGraphicsView(scene);
    splitter->addWidget(view);

    previewLabel = new QLabel();
    previewLabel->setMinimumSize(400, 300);
    splitter->addWidget(previewLabel);

    setCentralWidget(splitter);

    QToolBar* toolbar = addToolBar("Nodes");
    toolbar->addAction("Input Node", this, &MainWindow::addInputNode);
    toolbar->addAction("Output Node", this, &MainWindow::addOutputNode);
}

void MainWindow::setupMenu() {
    QMenu* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("Save Output", [this]() {
        QString file = QFileDialog::getSaveFileName(this, "Save Image");
        if (file.isEmpty()) return;

        QPixmap pm = previewLabel->pixmap();
        if (pm.isNull()) {
            QMessageBox::warning(this, "No Image", "Nothing to save!");
        } else {
            pm.save(file);
        }
    });
}

void MainWindow::addInputNode() {
    QString file = QFileDialog::getOpenFileName(this, "Open Image");
    if(file.isEmpty()) return;

    // Create GUI node
    inputItem = new NodeItem("Input", Qt::darkGreen, ++nodeCounter);
    scene->addItem(inputItem);

    // Create backend node
    inputNodePtr = std::make_shared<InputNode>();
    inputNodePtr->loadImage(file.toStdString());
    graph.addNode(inputNodePtr);

    // Connect if output exists
    if (outputNodePtr) {
        graph.connectNodes(inputNodePtr->id, outputNodePtr->id);
    }
    processGraph();
}

void MainWindow::addOutputNode() {
    outputItem = new NodeItem("Output", Qt::darkRed, ++nodeCounter);
    scene->addItem(outputItem);

    outputNodePtr = std::make_shared<OutputNode>();
    graph.addNode(outputNodePtr);

    if (inputNodePtr) {
        graph.connectNodes(inputNodePtr->id, outputNodePtr->id);
    }
    processGraph();
}

void MainWindow::processGraph() {
    if (!inputNodePtr || !outputNodePtr) return;
    try {
        outputNodePtr->process();
        cv::Mat result = outputNodePtr->getResult();
        if (result.empty()) return;
        QImage qimg(result.data, result.cols, result.rows,
                   result.step, QImage::Format_RGB888);
        previewLabel->setPixmap(QPixmap::fromImage(qimg));
    } catch(const std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
    }
}
