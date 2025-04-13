// ----------------- node_framework.h -----------------
#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <opencv2/opencv.hpp>

class Node {
public:
    struct Port {
        enum Direction { INPUT, OUTPUT };
        enum DataType  { IMAGE, PARAMETER };
        Direction direction;
        DataType type;
        std::string name;
        cv::Mat data;
        struct Connection { Node* node; int portIndex; };
        std::vector<Connection> connections;
    };

    Node(const std::string& name) : name(name), id(next_id++), dirty(true) {}
    virtual ~Node() = default;

    virtual void process() = 0;

    void connectTo(Node* target, int outputPort = 0, int inputPort = 0) {
        outputs[outputPort].connections.push_back({target, inputPort});
        target->inputs[inputPort].connections.push_back({this, outputPort});
        downstream.push_back(target);
        markDirty();
    }

    bool isDirty() const { return dirty; }
    void markClean() { dirty = false; }
    void markDirty() {
        if (!dirty) {
            dirty = true;
            for (auto* c : downstream) c->markDirty();
        }
    }

    std::string name;
    int id;
    std::vector<Port> inputs, outputs;
    std::vector<Node*> downstream;

    static int next_id;
protected:
    bool dirty;
};



// === Input Node ===
class InputNode : public Node {
public:
    InputNode() : Node("Image Input") {
        outputs.emplace_back(Port{Port::OUTPUT, Port::IMAGE, "Output"});
    }
    void loadImage(const std::string& path) {
        image = cv::imread(path);
        if (image.empty()) throw std::runtime_error("Failed to load image");
        cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
        markDirty();
    }
    void process() override {
        if (!isDirty()) return;
        if (!image.empty()) outputs[0].data = image.clone();
        markClean();
    }
private:
    cv::Mat image;
};

// === Output Node ===
class OutputNode : public Node {
public:
    OutputNode() : Node("Image Output") {
        inputs.emplace_back(Port{Port::INPUT, Port::IMAGE, "Input"});
    }
    void process() override {
        if (!isDirty()) return;
        if (!inputs[0].connections.empty()) {
            auto& c = inputs[0].connections[0];
            c.node->process();
            result = c.node->outputs[c.portIndex].data.clone();
        }
        markClean();
    }
    cv::Mat getResult() const { return result; }
private:
    cv::Mat result;
};

// === Brightness/Contrast Node ===
class BrightnessContrastNode : public Node {
public:
    BrightnessContrastNode()
      : Node("Brightness/Contrast"), brightness(0), contrast(1.0f)
    {
        inputs.emplace_back(Port{Port::INPUT, Port::IMAGE, "Input"});
        outputs.emplace_back(Port{Port::OUTPUT, Port::IMAGE, "Output"});
    }
    void setBrightness(int b)   { brightness = std::clamp(b, -100, 100); markDirty(); }
    void setContrast(float c)   { contrast = std::clamp(c, 0.0f, 3.0f); markDirty(); }
    void resetBrightness()      { brightness = 0; markDirty(); }
    void resetContrast()        { contrast = 1.0f; markDirty(); }
    void process() override {
        if (!isDirty()) return;
        if (inputs[0].connections.empty()) return;
        auto& c = inputs[0].connections[0];
        c.node->process();
        cv::Mat in = c.node->outputs[c.portIndex].data;
        if (in.empty()) return;
        cv::Mat out;
        in.convertTo(out, -1, contrast, brightness);
        outputs[0].data = out;
        markClean();
    }
private:
    int brightness;
    float contrast;
};

// === Blur Node ===
class BlurNode : public Node {
public:
    enum Mode { UNIFORM, DIRECTIONAL };
    BlurNode()
      : Node("Blur"), radius(1), mode(UNIFORM), angle(0.0f), amount(1.0f)
    {
        inputs.emplace_back(Port{Port::INPUT, Port::IMAGE, "Input"});
        outputs.emplace_back(Port{Port::OUTPUT, Port::IMAGE, "Output"});
    }
    void setRadius(int r)   { radius = std::clamp(r, 1, 20); markDirty(); }
    void setMode(Mode m)    { mode = m; markDirty(); }
    void setAngle(float a)  { angle = std::fmod(a,360.0f); markDirty(); }
    void setAmount(float a) { amount = std::clamp(a,0.0f,1.0f); markDirty(); }

    cv::Mat getKernel() const {
        int k = radius*2+1;
        if (mode==UNIFORM) {
            cv::Mat g = cv::getGaussianKernel(k,-1,CV_32F);
            return g * g.t();
        } else {
            cv::Mat M = cv::Mat::zeros(k,k,CV_32F);
            cv::Point2f c((k-1)/2.0f,(k-1)/2.0f);
            float rads = angle * CV_PI/180.0f;
            cv::Point2f d(std::cos(rads), std::sin(rads));
            for(int i=0;i<k;i++){
                float t = i-(k-1)/2.0f;
                cv::Point2f p = c + d*t;
                int x=std::round(p.x), y=std::round(p.y);
                if(x>=0&&x<k&&y>=0&&y<k) M.at<float>(y,x)=1;
            }
            return M / cv::sum(M)[0];
        }
    }

    void process() override {
        if (!isDirty()) return;
        if (inputs[0].connections.empty()) return;
        auto& c = inputs[0].connections[0];
        c.node->process();
        cv::Mat in = c.node->outputs[c.portIndex].data;
        if (in.empty()) return;
        cv::Mat blurred;
        int k = radius*2+1;
        if (mode==UNIFORM)
            cv::GaussianBlur(in, blurred, {k,k},0);
        else {
            cv::Mat K = getKernel();
            cv::filter2D(in, blurred, -1, K);
        }
        cv::Mat out;
        cv::addWeighted(blurred, amount, in, 1.0f-amount, 0, out);
        outputs[0].data = out;
        markClean();
    }
private:
    int radius;
    Mode mode;
    float angle, amount;
};

// === Node Graph ===
class NodeGraph {
public:
    void addNode(std::shared_ptr<Node> n) { nodes[n->id] = n; }
    void connectNodes(int s,int d,int o=0,int i=0){
        nodes[s]->connectTo(nodes[d].get(),o,i);
    }
    void propagateFrom(Node* start){
        std::queue<Node*> q;
        start->process(); start->markClean();
        q.push(start);
        while(!q.empty()){
            Node* cur = q.front(); q.pop();
            for(auto* c: cur->downstream){
                if(c->isDirty()){
                    c->process(); c->markClean();
                    q.push(c);
                }
            }
        }
    }
    std::unordered_map<int,std::shared_ptr<Node>> nodes;
};
