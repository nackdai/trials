#if !defined(__NODE_H__)
#define __NODE_H__

#include <stdint.h>
#include <thread>
#include "proxy.h"
#include "Exporter.h"
#include "JobQueue.h"

class Writer;

class Node {
public:
    Node() {}
    Node(
        Writer* writer,
        const Potree::AABB& aabb, int index, int level, float spacing);

    ~Node() {}

public:
    void init(
        Writer* writer,
        const Potree::AABB& aabb, int index, int level, float spacing);

    bool add(Potree::Point& point);

    bool flush(const std::string& base);

    void waitForFlush();

    void close();

private:
    Node* createChild(int index);

private:
    Writer* m_writer{ nullptr };

    Potree::AABB m_aabb;
    int m_index;
    int m_level;
    float m_spacing;

    // 追加された点群をぴったり内包するAABB.
    Potree::AABB m_acceptedAABB;

    Potree::SparseGrid m_grid;

    std::vector<Potree::Point> m_accepted;

    // 子ノード.
    std::vector<Node*> m_children;

    Exporter m_exporter;
    Event m_evExport;
};

#endif
