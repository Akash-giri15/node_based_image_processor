// ----------------- mainwindow.cpp -----------------
#include "mainwindow.h"

// --- NodeItem ---
NodeItem::NodeItem(Node* backend, QColor c)
  : backendNode(backend), color(c)
{
    setFlags(ItemIsMovable|ItemSendsGeometryChanges);
    if(!backend->inputs.empty()){
        auto* p=new PortItem(PortItem::In,this,0);
        inputs.push_back(p); p->setParentItem(this);
        p->setRect(-5,h/2-5,10,10);
    }
    if(!backend->outputs.empty()){
        auto* p=new PortItem(PortItem::Out,this,0);
        outputs.push_back(p); p->setParentItem(this);
        p->setRect(w-5,h/2-5,10,10);
    }
}

QRectF NodeItem::boundingRect() const { return {0,0,qreal(w),qreal(h)}; }

void NodeItem::paint(QPainter* p,const QStyleOptionGraphicsItem*,QWidget*){
    p->setBrush(color);
    p->drawRoundedRect(0,0,w,h,5,5);
    p->setPen(Qt::white);
    p->drawText(10,20,QString::fromStdString(backendNode->name));
}

// --- EdgeItem ---
EdgeItem::EdgeItem(PortItem* from) : fromPort(from) {
    setPen(QPen(Qt::white,2)); setZValue(-1);
    QPointF c=from->scenePos()+QPointF(5,5);
    updatePath(c);
}

void EdgeItem::setTarget(PortItem* to){
    toPort=to;
    updatePath(to->scenePos()+QPointF(5,5));
}

void EdgeItem::updatePath(const QPointF& to){
    QPointF f=fromPort->scenePos()+QPointF(5,5);
    QPainterPath path(f);
    path.lineTo(to);
    setPath(path);
}

// --- PortItem ---
PortItem::PortItem(PortType t,NodeItem* pn,int idx)
  : QGraphicsEllipseItem(), type(t), parentNode(pn), portIndex(idx)
{
    setBrush(type==Out?Qt::darkGreen:Qt::darkRed);
    setFlag(ItemSendsScenePositionChanges);
}

void PortItem::mousePressEvent(QGraphicsSceneMouseEvent* e){
    if(type==Out){
        tempEdge=new EdgeItem(this);
        scene()->addItem(tempEdge);
    }
    QGraphicsEllipseItem::mousePressEvent(e);
}

void PortItem::mouseMoveEvent(QGraphicsSceneMouseEvent* e){
    if(tempEdge) tempEdge->updatePath(e->scenePos());
    QGraphicsEllipseItem::mouseMoveEvent(e);
}

void PortItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* e) {
    if (tempEdge) {
        auto items = scene()->items(e->scenePos());
        for (auto* it : items) {
            if (auto* inP = qgraphicsitem_cast<PortItem*>(it)) {
                if (inP->type == In) {
                    Node* srcNode = parentNode->backendNode;
                    Node* dstNode = inP->parentNode->backendNode;

                    // find the MainWindow from the view
                    auto views = parentNode->scene()->views();
                    if (!views.isEmpty()) {
                        if (auto* mw = qobject_cast<MainWindow*>(views.first()->window())) {
                            mw->graph.connectNodes(srcNode->id, dstNode->id);
                            mw->graph.propagateFrom(srcNode);
                            mw->processGraph();
                        }
                    }

                    tempEdge->setTarget(inP);
                    break;
                }
            }
        }
        if (!tempEdge->toPort) {
            scene()->removeItem(tempEdge);
            delete tempEdge;
        }
        tempEdge = nullptr;
    }
    QGraphicsEllipseItem::mouseReleaseEvent(e);
}
// --- MainWindow ---
MainWindow::MainWindow(QWidget* p):QMainWindow(p){
    setupUI(); setupMenu();
}

void MainWindow::setupUI(){
    auto* sp=new QSplitter(this);
    scene=new QGraphicsScene(this);
    sp->addWidget(new QGraphicsView(scene));
    previewLabel=new QLabel(); previewLabel->setMinimumSize(400,300);
    sp->addWidget(previewLabel);
    setCentralWidget(sp);

    auto* tb=addToolBar("Nodes");
    tb->addAction("Input Node", [=](){
        QString f=QFileDialog::getOpenFileName(this,"Open Image");
        if(f.isEmpty())return;
        inputNodePtr=std::make_shared<InputNode>();
        inputNodePtr->loadImage(f.toStdString());
        graph.addNode(inputNodePtr);
        auto* ni=new NodeItem(inputNodePtr.get(),Qt::darkGreen);
        scene->addItem(ni);
        if(outputNodePtr) graph.connectNodes(inputNodePtr->id,outputNodePtr->id);
        processGraph();
    });
    tb->addAction("Output Node", [=](){
        outputNodePtr=std::make_shared<OutputNode>();
        graph.addNode(outputNodePtr);
        auto* ni=new NodeItem(outputNodePtr.get(),Qt::darkRed);
        scene->addItem(ni);
        if(inputNodePtr) graph.connectNodes(inputNodePtr->id,outputNodePtr->id);
        processGraph();
    });
    tb->addAction("Brightness/Contrast Node", [=](){
        bcNodePtr=std::make_shared<BrightnessContrastNode>();
        graph.addNode(bcNodePtr);
        auto* ni=new NodeItem(bcNodePtr.get(),Qt::blue);
        scene->addItem(ni);
        if(inputNodePtr)  graph.connectNodes(inputNodePtr->id, bcNodePtr->id);
        if(outputNodePtr) graph.connectNodes(bcNodePtr->id, outputNodePtr->id);
        setupBCControls();
        graph.propagateFrom(bcNodePtr.get());
        processGraph();
    });
    tb->addAction("Blur Node", [=](){
        blurNodePtr=std::make_shared<BlurNode>();
        graph.addNode(blurNodePtr);
        auto* ni=new NodeItem(blurNodePtr.get(),Qt::magenta);
        scene->addItem(ni);
        if(inputNodePtr)  graph.connectNodes(inputNodePtr->id, blurNodePtr->id);
        if(outputNodePtr) graph.connectNodes(blurNodePtr->id, outputNodePtr->id);
        setupBlurControls();
        graph.propagateFrom(blurNodePtr.get());
        processGraph();
    });
}

void MainWindow::setupMenu(){
    auto* m=menuBar()->addMenu("File");
    m->addAction("Save Output", [=](){
        QString f=QFileDialog::getSaveFileName(this,"Save Image");
        if(f.isEmpty())return;
        QPixmap pm=previewLabel->pixmap();
        if(pm.isNull()) QMessageBox::warning(this,"No Image","Nothing to save!");
        else pm.save(f);
    });
}

void MainWindow::setupBCControls(){
    if(bcWidget) delete bcWidget;
    bcWidget=new QWidget(this);
    auto* L=new QVBoxLayout(bcWidget);
    L->addWidget(new QLabel("Brightness"));
    bSlider=new QSlider(Qt::Horizontal); bSlider->setRange(-100,100); bSlider->setValue(0);
    L->addWidget(bSlider);
    rbBtn=new QPushButton("Reset"); L->addWidget(rbBtn);
    L->addWidget(new QLabel("Contrast"));
    cSlider=new QSlider(Qt::Horizontal); cSlider->setRange(0,300); cSlider->setValue(100);
    L->addWidget(cSlider);
    rcBtn=new QPushButton("Reset"); L->addWidget(rcBtn);

    connect(bSlider,&QSlider::valueChanged,this,[&](int v){
        bcNodePtr->setBrightness(v);
        graph.propagateFrom(bcNodePtr.get());
        processGraph();
    });
    connect(rbBtn,&QPushButton::clicked,this, [&](){ bSlider->setValue(0); });
    connect(cSlider,&QSlider::valueChanged,this,[&](int v){
        bcNodePtr->setContrast(v/100.0f);
        graph.propagateFrom(bcNodePtr.get());
        processGraph();
    });
    connect(rcBtn,&QPushButton::clicked,this,[&](){ cSlider->setValue(100); });

    auto* dock=new QDockWidget("Brightness/Contrast",this);
    dock->setWidget(bcWidget);
    addDockWidget(Qt::RightDockWidgetArea,dock);
}

void MainWindow::setupBlurControls(){
    if(blurWidget) delete blurWidget;
    blurWidget=new QWidget(this);
    auto* L=new QVBoxLayout(blurWidget);
    L->addWidget(new QLabel("Radius"));
    rSlider=new QSlider(Qt::Horizontal); rSlider->setRange(1,20); rSlider->setValue(1);
    L->addWidget(rSlider);
    L->addWidget(new QLabel("Mode"));
    modeCombo=new QComboBox(); modeCombo->addItems({"Uniform","Directional"});
    L->addWidget(modeCombo);
    L->addWidget(new QLabel("Angle"));
    aSlider=new QSlider(Qt::Horizontal); aSlider->setRange(0,360); aSlider->setValue(0);
    aSlider->setEnabled(false); L->addWidget(aSlider);
    L->addWidget(new QLabel("Amount"));
    amtSlider=new QSlider(Qt::Horizontal); amtSlider->setRange(0,100); amtSlider->setValue(100);
    L->addWidget(amtSlider);
    L->addWidget(new QLabel("Kernel Preview"));
    kernelTable=new QTableWidget(); L->addWidget(kernelTable);

    connect(rSlider,&QSlider::valueChanged,this,[&](int v){
        blurNodePtr->setRadius(v);
        updateKernelPreview();
        graph.propagateFrom(blurNodePtr.get());
        processGraph();
    });
    connect(modeCombo,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[&](int i){
        blurNodePtr->setMode((BlurNode::Mode)i);
        aSlider->setEnabled(i==BlurNode::DIRECTIONAL);
        updateKernelPreview();
        graph.propagateFrom(blurNodePtr.get());
        processGraph();
    });
    connect(aSlider,&QSlider::valueChanged,this,[&](int v){
        blurNodePtr->setAngle(v);
        updateKernelPreview();
        graph.propagateFrom(blurNodePtr.get());
        processGraph();
    });
    connect(amtSlider,&QSlider::valueChanged,this,[&](int v){
        blurNodePtr->setAmount(v/100.0f);
        graph.propagateFrom(blurNodePtr.get());
        processGraph();
    });

    auto* dock=new QDockWidget("Blur Controls",this);
    dock->setWidget(blurWidget);
    addDockWidget(Qt::RightDockWidgetArea,dock);
    updateKernelPreview();
}

void MainWindow::updateKernelPreview(){
    cv::Mat K=blurNodePtr->getKernel();
    int r=K.rows,c=K.cols;
    kernelTable->clear(); kernelTable->setRowCount(r); kernelTable->setColumnCount(c);
    for(int i=0;i<r;i++)for(int j=0;j<c;j++){
        float v=K.at<float>(i,j);
        auto* it=new QTableWidgetItem(QString::number(v,'f',3));
        it->setTextAlignment(Qt::AlignCenter);
        kernelTable->setItem(i,j,it);
    }
    kernelTable->resizeColumnsToContents();
    kernelTable->resizeRowsToContents();
}

void MainWindow::processGraph(){
    if(!inputNodePtr||!outputNodePtr) return;
    try{
        outputNodePtr->process();
        cv::Mat img=outputNodePtr->getResult();
        if(img.empty()) return;
        QImage qi(img.data,img.cols,img.rows,img.step,QImage::Format_RGB888);
        previewLabel->setPixmap(QPixmap::fromImage(qi));
    } catch(const std::exception& e){
        QMessageBox::critical(this,"Error",e.what());
    }
}
