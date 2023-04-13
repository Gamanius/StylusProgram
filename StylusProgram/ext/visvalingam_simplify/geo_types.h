//
//
// 2013 (c) Mathieu Courtemanche

#ifndef GEO_TYPES_H
#define GEO_TYPES_H
#include <vector>
#include "util/Util.h"
#include <string>

class OGRPoint;
class OGRLineString;
class OGRLinearRing;
class OGRvs_Polygon;
class OGRMultivs_Polygon;

typedef size_t VertexIndex;

typedef std::vector<Point2D<double>> Linestring;
typedef std::vector<Linestring> MultiLinestring;

struct vs_Polygon
{
    vs_Polygon() {}

    Linestring exterior_ring;
    MultiLinestring interior_rings;
};
typedef std::vector<vs_Polygon> Multivs_Polygon;

// returns cross product between two vectors: v1 ^ v2 in right handed coordinate
// E.g.: returned value on +z axis
double cross_product(const Point2D<double>& v1, const Point2D<double>& v2);

// A - B
Point2D<double> vector_sub(const Point2D<double>& A, const Point2D<double>& B);

#endif // GEO_TYPES_H
