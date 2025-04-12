#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <opencv2/opencv.hpp>

// ==================== Base Node ====================
class Node {
public:
    struct Port {
        enum Direction { INPUT, OUTPUT };
        enum DataType { IMAGE, PARAMETER };

        Direction direction;
        DataType type;
        std::string name;
        cv::Mat data;

        struct Connection {
            Node* node;
            int portIndex;
        };
        std::vector<Connection> connections;
    };

    Node(const std::string& name) : name(name), id(next_id++), dirty(true) {}
    virtual ~Node() = default;
    virtual void process() = 0;

    void connectTo(Node* target, int outputPort = 0, int inputPort = 0) {
        if (!validateConnection(target, outputPort, inputPort))
            throw std::runtime_error("Invalid connection");
        outputs[outputPort].connections.push_back({target, inputPort});
        target->inputs[inputPort].connections.push_back({this, outputPort});
        markDirty();
    }

    bool isDirty() const { return dirty; }
    void markClean() { dirty = false; }
    void markDirty() {
        dirty = true;
        for (auto& out : outputs)
            for (auto& conn : out.connections)
                conn.node->markDirty();
    }

    std::string name;
    int id;
    std::vector<Port> inputs;
    std::vector<Port> outputs;

    static int next_id;    // declaration only

protected:
    bool validateConnection(Node* target, int outputPort, int inputPort) {
        if (outputPort >= outputs.size() || inputPort >= target->inputs.size())
            return false;
        if (outputs[outputPort].direction != Port::OUTPUT ||
            target->inputs[inputPort].direction != Port::INPUT)
            return false;
        if (outputs[outputPort].type != target->inputs[inputPort].type)
            return false;
        return !hasDirectCycle(target);
    }

    bool hasDirectCycle(Node* target) {
        for (auto& conn : target->outputs)
            for (auto& tconn : conn.connections)
                if (tconn.node == this) return true;
        return false;
    }

    bool dirty;
};

// ==================== Input Node ====================
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
        if (!image.empty())
            outputs[0].data = image.clone();
        markClean();
    }

private:
    cv::Mat image;
};

// ==================== Output Node ====================
class OutputNode : public Node {
public:
    OutputNode() : Node("Image Output"), format("PNG"), quality(95) {
        inputs.emplace_back(Port{Port::INPUT, Port::IMAGE, "Input"});
    }

    void process() override {
        if (!isDirty()) return;
        if (!inputs[0].connections.empty()) {
            auto& src = inputs[0].connections[0];
            src.node->process();
            result = src.node->outputs[src.portIndex].data.clone();
        }
        markClean();
    }

    void saveResult(const std::string& path) {
        if (result.empty()) throw std::runtime_error("No result to save");
        std::vector<int> params;
        if (format == "JPG")
            params = {cv::IMWRITE_JPEG_QUALITY, quality};
        else // PNG
            params = {cv::IMWRITE_PNG_COMPRESSION, quality / 10};
        cv::imwrite(path, result, params);
    }

    cv::Mat getResult() const { return result; }

private:
    cv::Mat result;
    std::string format;
    int quality;
};

// ==================== Node Graph ====================
class NodeGraph {
public:
    void addNode(std::shared_ptr<Node> node) {
        nodes[node->id] = node;
    }

    void connectNodes(int srcId, int dstId, int outPort = 0, int inPort = 0) {
        auto src = nodes[srcId].get();
        auto dst = nodes[dstId].get();
        if (hasCycle(dst, src))
            throw std::runtime_error("Connection would create cycle");
        src->connectTo(dst, outPort, inPort);
    }

private:
    bool hasCycle(Node* start, Node* target) {
        if (start == target) return true;
        for (auto& out : start->outputs)
            for (auto& conn : out.connections)
                if (hasCycle(conn.node, target)) return true;
        return false;
    }

    std::unordered_map<int, std::shared_ptr<Node>> nodes;
};
