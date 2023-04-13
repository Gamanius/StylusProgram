//
//
// 2013 (c) Mathieu Courtemanche
//
#include "geo_types.h"
#include <cassert>
#include <sstream>

double cross_product(const Point2D<double>& v1, const Point2D<double>& v2)
{
    return (v1.x * v2.y) - (v1.y * v2.x);
}

Point2D<double> vector_sub(const Point2D<double>& A, const Point2D<double>& B)
{
    Point2D<double> res;
    res.x = A.x - B.x;
    res.y = A.y - B.y;
    return res;
}

