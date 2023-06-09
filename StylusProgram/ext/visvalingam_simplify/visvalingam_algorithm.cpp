//
//
// 2013 (c) Mathieu Courtemanche

#include "visvalingam_algorithm.h"
#include <cstdlib>
#include <cmath>
#include <limits>
#include <vector>
#include "heap.hpp"

#undef max

static const double NEARLY_ZERO = 1e-7;

// Represents 3 vertices from the input line and its associated effective area.
struct VertexNode
{
    VertexNode(VertexIndex vertex_, VertexIndex prev_vertex_,
               VertexIndex next_vertex_, double area_)
        : vertex(vertex_)
        , prev_vertex(prev_vertex_)
        , next_vertex(next_vertex_)
        , area(area_)
    {
    }

    // ie: a triangle
    VertexIndex vertex;
    VertexIndex prev_vertex;
    VertexIndex next_vertex;
    // effective area
    double area;
};

struct VertexNodeCompare
{
    bool operator()(const VertexNode* lhs, const VertexNode* rhs) const
    {
        return lhs->area < rhs->area;
    }
};

static double effective_area(VertexIndex current, VertexIndex previous,
                            VertexIndex next, const Linestring& input_line)
{
    const Point2D<double>& c = input_line[current];
    const Point2D<double>& p = input_line[previous];
    const Point2D<double>& n = input_line[next];
    const Point2D<double> c_n = vector_sub(n, c);
    const Point2D<double> c_p = vector_sub(p, c);
    const double det = cross_product(c_n, c_p);
    return 0.5 * fabs(det);
}

static double effective_area(const VertexNode& node,
                            const Linestring& input_line)
{
    return effective_area(node.vertex, node.prev_vertex,
                            node.next_vertex, input_line);
}

Visvalingam_Algorithm::Visvalingam_Algorithm(const Linestring& input)
    : m_effective_areas(input.size(), 0.0)
    , m_input_line(input)
{
    // Compute effective area for each Point2D<double> in the input (except endPoint2D<double>s)
    std::vector<VertexNode*> node_list(input.size(), NULL);
    Heap<VertexNode*, VertexNodeCompare> min_heap(input.size());
    for (VertexIndex i=1; i < input.size()-1; ++i)
    {
        double area = effective_area(i, i-1, i+1, input);
        if (area > NEARLY_ZERO)
        {
            node_list[i] = new VertexNode(i, i-1, i+1, area);
            min_heap.insert(node_list[i]);
        }
    }

    double min_area = -std::numeric_limits<double>::max();
    while (!min_heap.empty())
    {
        VertexNode* curr_node = min_heap.pop();
        assert (curr_node == node_list[curr_node->vertex]);

        // If the current Point2D<double>'s calculated area is less than that of the last
        // Point2D<double> to be eliminated, use the latter's area instead. (This ensures
        // that the current Point2D<double> cannot be eliminated without eliminating
        // previously eliminated Point2D<double>s.)
        min_area = std::max(min_area, curr_node->area);

        VertexNode* prev_node = node_list[curr_node->prev_vertex];
        if (prev_node != NULL)
        {
            prev_node->next_vertex = curr_node->next_vertex;
            prev_node->area = effective_area(*prev_node, input);
            min_heap.reheap(prev_node);
        }
        
        VertexNode* next_node = node_list[curr_node->next_vertex];
        if (next_node != NULL)
        {
            next_node->prev_vertex = curr_node->prev_vertex;
            next_node->area = effective_area(*next_node, input);
            min_heap.reheap(next_node);
        }

        // store the final value for this vertex and delete the node.
        m_effective_areas[curr_node->vertex] = min_area;
        node_list[curr_node->vertex] = NULL;
        delete curr_node;
    }
    node_list.clear();
}

void Visvalingam_Algorithm::simplify(double area_threshold,
                                    Linestring* res) const
{
    assert(res);
    for (VertexIndex i=0; i < m_input_line.size(); ++i)
    {
        if (contains_vertex(i, area_threshold))
        {
            res->push_back(m_input_line[i]);
        }
    }
    if (res->size() < 4)
    {
        res->clear();
    }
}
