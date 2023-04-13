#pragma once
#include <cmath>
#include <string>
#include <vector>
#include <chrono>

#include <d2d1.h>
#include "mupdf/fitz.h"

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

	T distance() {
		return std::sqrt(x * x + y * y);
	}

	T distance(Point2D<T> p) {
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

	Rect2D(RECT r) {
		upperleft.x = r.left;
		upperleft.y = r.top;
		width = r.right - r.left;
		height = r.bottom - r.top;
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
		return fz_make_rect((T)upperleft.x, (T)upperleft.y, (T)(upperleft.x + width), (T)(upperleft.y + height));
	}


	// create a function that returns true if two rectangles intersect
	bool intersects(const Rect2D<T>& other) const {
		return (upperleft.x < other.upperleft.x + other.width &&
							upperleft.x + width > other.upperleft.x &&
							upperleft.y < other.upperleft.y + other.height &&
							upperleft.y + height > other.upperleft.y);
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

template <class T> void SafeRelease(T** ppT) {
	if (*ppT) {
		(*ppT)->Release();
		*ppT = NULL;
	}
}

template <typename T>
bool isEqual(T a, T b, T epsilon = 1e-9) {
	return std::fabs(a - b) < epsilon;
}
