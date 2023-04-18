#pragma once
#include <cmath>
#include <string>
#include <vector>
#include <chrono>

#include <d2d1.h>
#include "mupdf/fitz.h"
#include <cmath>

typedef unsigned char byte;

template <typename T>
struct Point2D {
	T x = 0;
	T y = 0;

	Point2D() = default;
	Point2D(const Point2D& p) = default;
	Point2D& operator=(const Point2D& p) = default;
	Point2D(Point2D&& p) = default;
	Point2D& operator=(Point2D&& p) = default;

	Point2D(T x, T y) : y(y), x(x) {}
	Point2D(POINT p) : y(p.y), x(p.x) {}
	Point2D(D2D1_POINT_2F p) : y(p.y), x(p.x) {}
	Point2D(fz_point p) : y(p.y), x(p.x) {}

	template <typename G>
	operator Point2D<G>() const {
		Point2D<G> p;
		p.x = (G)x;
		p.y = (G)y;
		return p;
	}

	operator std::wstring() const {
		std::wstring s;
		s += L"[x: " + std::to_wstring(x) + L" y: " + std::to_wstring(y) + L"]";
		return s;
	}

	operator fz_point() const {
		return fz_make_point((float)x, (float)y);
	}

	template<typename G>
	Point2D<T>& operator*=(const Point2D<G>& other) {
		x *= (T)other.x;
		y *= (T)other.y;
		return *this;
	}

	template<typename G>
	Point2D<T>& operator/=(const Point2D<G>& other) {
		x /= (T)other.x;
		y /= (T)other.y;
		return *this;
	}

	template<typename G>
	Point2D<T>& operator*=(const G& other) {
		x *= (T)other;
		y *= (T)other;
		return *this;
	}

	template<typename G>
	Point2D<T>& operator+=(const Point2D<G>& other) {
		x += (T)other.x;
		y += (T)other.y;
		return *this;
	}

	template<typename G>
	Point2D<T>& operator-=(const Point2D<G>& other) {
		x -= (T)other.x;
		y -= (T)other.y;
		return *this;
	}

	operator D2D1_POINT_2F() const {
		return D2D1::Point2F(x, y);
	}

	operator D2D1_SIZE_F() const {
		return D2D1::SizeF(x, y);
	}

	Point2D<T> operator-() const {
		return Point2D<T>(-x, -y);
	}

	T distance() const {
		return std::sqrt(x * x + y * y);
	}

	T distance(Point2D<T> p) const {
		return std::sqrt((p.x - x) * (p.x - x) + (p.y - y) * (p.y - y));
	}
};


template<typename T, typename G>
inline Point2D<T> operator*(const G other, const Point2D<T>& point) {
	Point2D<T> result = point;
	result *= other;
	return result;
}
template<typename T, typename G>
inline Point2D<T> operator*(const Point2D<T>& point, const G other) {
	Point2D<T> result = point;
	result *= other;
	return result;
}

template<typename T, typename G>
inline Point2D<T> operator*(const Point2D<G>& point1, const Point2D<T>& point2) {
	Point2D<T> result = point1;
	result *= point2;
	return result;
}

template<typename T, typename G>
inline Point2D<T> operator/(const G other, const Point2D<T>& point) {
	Point2D<T> result = point;
	result *= 1/other;
	return result;
}

template<typename T, typename G>
inline Point2D<T> operator/(const Point2D<T>& point, const G other) {
	Point2D<T> result = point;
	result *= 1 / other;
	return result;
}

template<typename T, typename G>
inline Point2D<T> operator+(const Point2D<G>& point1, const Point2D<T>& point2) {
	Point2D<T> result = point1;
	result += point2;
	return result;
}

template<typename T, typename G>
inline Point2D<T> operator-(const Point2D<G>& point1, const Point2D<T>& point2) {
	Point2D<T> result = point1;
	result -= point2;
	return result;
}

// compute the distance from point p3 to the line defined by p1 and p2. The line is NOT assumed to be infinite.
template <typename T>
inline double pointToLineDistance(const Point2D<T>& p1, const Point2D<T>& p2, const Point2D<T>& p3) {
	// Compute the dot product of vectors (p3-p1) and (p2-p1)
	double dot = (p3.x - p1.x) * (p2.x - p1.x) + (p3.y - p1.y) * (p2.y - p1.y);

	// Compute the length of vector (p2-p1)
	double length = std::sqrt((p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y));

	// Compute the distance from point p3 to the line passing through p1 and p2
	double distance;

	if (length == 0) { // Handle the case where p1 and p2 coincide
		distance = std::sqrt((p3.x - p1.x) * (p3.x - p1.x) + (p3.y - p1.y) * (p3.y - p1.y));
	}
	else {
		double t = dot / (length * length);
		if (t < 0) { // Handle the case where the closest point is p1
			distance = std::sqrt((p3.x - p1.x) * (p3.x - p1.x) + (p3.y - p1.y) * (p3.y - p1.y));
		}
		else if (t > 1) { // Handle the case where the closest point is p2
			distance = std::sqrt((p3.x - p2.x) * (p3.x - p2.x) + (p3.y - p2.y) * (p3.y - p2.y));
		}
		else { // Handle the case where the closest point is between p1 and p2
			Point2D<T> closest(p1.x + t * (p2.x - p1.x), p1.y + t * (p2.y - p1.y));
			distance = std::sqrt((p3.x - closest.x) * (p3.x - closest.x) + (p3.y - closest.y) * (p3.y - closest.y));
		}
	}

	return distance;
}

template <typename T>
double distanceBetweenLines(const Point2D<T>& p1, const Point2D<T>& p2, const Point2D<T>& p3, const Point2D<T>& p4) {
	// Compute slopes and y-intercepts of the two lines
	double m1 = (p2.y - p1.y) / (p2.x - p1.x);
	double b1 = p1.y - m1 * p1.x;

	double m2 = (p4.y - p3.y) / (p4.x - p3.x);
	double b2 = p3.y - m2 * p3.x;

	// Compute the intersection point of the two lines
	double x_intersect = (b2 - b1) / (m1 - m2);
	double y_intersect = m1 * x_intersect + b1;

	// Check if the intersection point lies on both line segments
	bool on_segment_1 = x_intersect >= min(p1.x, p2.x) && x_intersect <= max(p1.x, p2.x)
		&& y_intersect >= min(p1.y, p2.y) && y_intersect <= max(p1.y, p2.y);

	bool on_segment_2 = x_intersect >= min(p3.x, p4.x) && x_intersect <= max(p3.x, p4.x)
		&& y_intersect >= min(p3.y, p4.y) && y_intersect <= max(p3.y, p4.y);

	if (!on_segment_1 || !on_segment_2) {
		// The intersection point does not lie on both line segments, so compute the distance to the closest endpoint
		double distance1 = min(pointToPointDistance(p1, p3), pointToPointDistance(p1, p4));
		double distance2 = min(pointToPointDistance(p2, p3), pointToPointDistance(p2, p4));
		return min(distance1, distance2);
	}

	// Compute the distance between the intersection point and each line
	double distance1 = pointToLineDistance(p1, p2, Point2D<T>(x_intersect, y_intersect));
	double distance2 = pointToLineDistance(p3, p4, Point2D<T>(x_intersect, y_intersect));

	// Return the minimum distance
	return min(distance1, distance2);
}

template <typename T>
struct Rect2D {
	Point2D<T> upperleft;
	T width;
	T height;

	Rect2D() = default;
	Rect2D(const Rect2D& p) = default;
	Rect2D& operator=(const Rect2D& p) = default;
	Rect2D(Rect2D&& p) = default;
	Rect2D& operator=(Rect2D&& p) = default;

	Rect2D(Point2D<T> p, T width, T height) : upperleft(p), width(width), height(height) {}
	Rect2D(Point2D<T> p1, Point2D<T> p2) {
		upperleft.x = min(p1.x, p2.x);
		upperleft.y = min(p1.y, p2.y);
		width = std::fabs(p1.x - p2.x);
		height = std::fabs(p1.y - p2.y);
	}

	Rect2D(RECT r) {
		upperleft.x = r.left;
		upperleft.y = r.top;
		width = r.right - r.left;
		height = r.bottom - r.top;
	}

	Rect2D(fz_rect r) {
		upperleft.x = r.x0;
		upperleft.y = r.y0;
		width = r.x1 - r.x0;
		height = r.y1 - r.y0;
	}

	template <typename G>
	Rect2D(Rect2D<G> r) {
		upperleft.x = (T)r.upperleft.x;
		upperleft.y = (T)r.upperleft.y;
		width = (T)r.width;
		height = (T)r.height;
	}

	T area() const {
		return width * height;
	}

	operator RECT() const {
		RECT r;
		r.left = upperleft.x;
		r.top = upperleft.y;
		r.right = upperleft.x + width;
		r.bottom = upperleft.y + height;
		return r;
	}

	operator D2D1_RECT_F() const {
		D2D1_RECT_F r;
		r.left = upperleft.x;
		r.top = upperleft.y;
		r.right = upperleft.x + width;
		r.bottom = upperleft.y + height;
		return r;
	}

	operator fz_irect() const {
		return fz_make_irect((T)upperleft.x, (T)upperleft.y, (T)(upperleft.x + width), (T)(upperleft.y + height));
	}

	operator fz_rect() const {
		return fz_make_rect((float)upperleft.x, (float)upperleft.y, (float)(upperleft.x + width), (float)(upperleft.y + height));
	}
	
	bool intersects(const Rect2D<T>& other) const {
		return (upperleft.x < other.upperleft.x + other.width &&
							upperleft.x + width > other.upperleft.x &&
							upperleft.y < other.upperleft.y + other.height &&
							upperleft.y + height > other.upperleft.y);
	}

	bool intersects(const Point2D<T>& p) const {
		return (p.x >= upperleft.x && p.x <= upperleft.x + width && p.y >= upperleft.y && p.y <= upperleft.y + height);
	}

	// calculate the intersection of two rectangles
	Rect2D<T> intersection(const Rect2D<T>& other) const {
		Rect2D<T> result;
		result.upperleft.x = max(upperleft.x, other.upperleft.x);
		result.upperleft.y = max(upperleft.y, other.upperleft.y);
		result.width = min(upperleft.x + width, other.upperleft.x + other.width) - result.upperleft.x;
		result.height = min(upperleft.y + height, other.upperleft.y + other.height) - result.upperleft.y;
		return result;
	}

	RECT toNormalizedRect() const {
		RECT r;
		r.left = 0;
		r.top = 0;
		r.right = width;
		r.bottom = height;
		return r;
	}

	operator std::wstring() const {
		std::wstring s;
		s += L"[x: " + std::to_wstring(upperleft.x) + L" y: " + std::to_wstring(upperleft.y) + L" width: " + std::to_wstring(width) + L" height: " + std::to_wstring(height) + L"]";
		return s;
	}
};

// rect2d + point2d operator
template <typename T>
inline Rect2D<T> operator+(const Rect2D<T>& rect, const Point2D<T>& point) {
	return Rect2D<T>(rect.upperleft + point, rect.width, rect.height);
}


inline Rect2D<float> operator*(const Rect2D<float>& rect, float scale) {
	return Rect2D<float>(rect.upperleft * scale, rect.width * scale, rect.height * scale);
}

inline UINT64 TimeSince1970() {
	auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
	return ms.time_since_epoch().count();
}

template <class T>
void SafeRelease(T** ppT) {
	if (*ppT) {
		(*ppT)->Release();
		*ppT = NULL;
	}
}

template <typename T>
bool isEqual(T a, T b, T epsilon = 1e-9) {
	return std::fabs(a - b) < epsilon;
}
