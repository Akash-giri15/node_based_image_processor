#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <opencv2/opencv.hpp>

// ==================== Enhanced Node Framework ====================
class Node {
public:
    struct Port {
        enum Direction { INPUT, OUTPUT };
        enum DataType { IMAGE, PARAMETER };

        Direction direction;
        DataType type;
        std::string name;
        cv::Mat data;

        // Connections: Each port can have multiple connections
        struct Connection {
            Node* node;       // Connected node
            int portIndex;    // Port index on the connected node
        };
        std::vector<Connection> connections; // List of connections for this port
    };

    Node(const std::string& name) : name(name), id(next_id++), dirty(true) {}
    virtual ~Node() = default;

    virtual void process() = 0;

    // Connection management
    void connectTo(Node* target, int outputPort = 0, int inputPort = 0) {
        if (!validateConnection(target, outputPort, inputPort)) {
            throw std::runtime_error("Invalid connection");
        }
        outputs[outputPort].connections.push_back({target, inputPort});
        target->inputs[inputPort].connections.push_back({this, outputPort});
        markDirty();
    }

    // Caching and state management
    bool isDirty() const { return dirty; }
    void markClean() { dirty = false; }
    void markDirty() {
        dirty = true;
        for (auto& out : outputs) {
            for (auto& conn : out.connections) {
                conn.node->markDirty();
            }
        }
    }

    // Common interface
    std::string name;
    int id;
    std::vector<Port> inputs;
    std::vector<Port> outputs;

    static int next_id;

protected:
    bool validateConnection(Node* target, int outputPort, int inputPort) {
        if (outputPort >= outputs.size() || inputPort >= target->inputs.size())
            return false;
        if (outputs[outputPort].direction != Port::OUTPUT)
            return false;
        if (target->inputs[inputPort].direction != Port::INPUT)
            return false;
        if (outputs[outputPort].type != target->inputs[inputPort].type)
            return false;

        return !hasDirectCycle(target);
    }

    bool hasDirectCycle(Node* target) {
        for (auto& conn : target->outputs) {
            for (auto& tconn : conn.connections) {
                if (tconn.node == this) return true;
            }
        }
        return false;
    }

    bool dirty; // Cache validation flag
};
int Node::next_id = 0;

// ==================== Enhanced Input Node ====================
class InputNode : public Node {
public:
    InputNode() : Node("Image Input") {
        // Configure ports
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

        if (!image.empty()) {
            outputs[0].data = image.clone();
        }
        markClean();
    }

private:
    cv::Mat image;
};

// ==================== Enhanced Output Node ====================
class OutputNode : public Node {
public:
    OutputNode() : Node("Image Output") {
        // Configure ports
        inputs.emplace_back(Port{Port::INPUT, Port::IMAGE, "Input"});
        
        format = "PNG";
        quality = 95;
    }

    void process() override {
        if (!isDirty()) return;

        if (!inputs[0].connections.empty()) {
            auto& source = inputs[0].connections[0];
            source.node->process();
            result = source.node->outputs[source.portIndex].data.clone();
        }

        markClean();
    }

    void saveResult(const std::string& path) {
        if (result.empty()) throw std::runtime_error("No result to save");

        std::vector<int> params;
        if (format == "JPG") {
            params = {cv::IMWRITE_JPEG_QUALITY, quality};
        } else if (format == "PNG") {
            params = {cv::IMWRITE_PNG_COMPRESSION, quality / 10};
        }

        cv::imwrite(path, result, params);
    }

private:
    cv::Mat result;
    std::string format;
    int quality;
};

// ==================== Enhanced Node Graph ====================
class NodeGraph {
public:
    void addNode(std::shared_ptr<Node> node) {
        nodes[node->id] = node;
    }

    void connectNodes(int sourceId, int destId,
                      int outPort = 0, int inPort = 0) {
        auto source = nodes[sourceId].get();
        auto dest = nodes[destId].get();

        if (hasCycle(dest, source)) {
            throw std::runtime_error("Connection would create cycle");
        }

        source->connectTo(dest, outPort, inPort);
    }

private:
    bool hasCycle(Node* start, Node* target) {
        if (start == target) return true;
        
        for (auto& out : start->outputs) {
            for (auto& conn : out.connections) {
                if (hasCycle(conn.node, target)) return true;
            }
        }
        
        return false;
    }

private:
    std::unordered_map<int, std::shared_ptr<Node>> nodes;
};
