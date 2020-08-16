#pragma once

#include "Constants.h"

namespace VE {
	class Bounds {
	public:
		float x0, y0, x1, y1;

		Bounds() {
			x0 = 0; y0 = 0; x1 = 0; y1 = 0;
		};
		Bounds(float x0, float y0, float x1, float y1) {
			this->x0 = x0;
			this->y0 = y0;
			this->x1 = x1;
			this->y1 = y1;
		};
		Bounds(const Bounds& other) {
			x0 = other.x0;
			y0 = other.y0;
			x1 = other.x1;
			y1 = other.y1;
		};

		inline float Width() const {
			return x1 - x0;
		};
		inline float Height() const {
			return y1 - y0;
		};
		Point Center() const {
			return Point(x0 + (x1 - x0) / 2, y0 + (y1 - y0) / 2);
		};

		inline bool Overlap(const Bounds& other) const {
			return !(other.x0 > x1 ||
				other.y0 > y1 ||
				other.x1 < x0 ||
				other.y1 < y0);
		};
		inline bool Contains(const Point& pt) const {
			return (pt.x >= x0 && pt.x <= x1 &&
				pt.y >= y0 && pt.y <= y1);
		};
		inline bool Contains(const Bounds& other) const {
			return (x0 < other.x0 &&
				y0 < other.y0 &&
				x1 > other.x1 &&
				y1 > other.y1);
		};
		inline void Pad(const float& padding) {
			x0 -= padding; y0 -= padding;
			x1 += padding; y1 += padding;
		};

		inline float Area() const {
			return (x1 - x0) * (y1 - y0);
		};

		inline cv::Rect2f Rect() const {
			return cv::Rect2f(x0, y0, x1 - x0, y1 - y0);
		};

		friend std::ostream& operator<<(std::ostream& os, const Bounds& b)
		{
			os << "[" << b.x0 << ", " << b.y0 << " - " << b.x1 << ", " << b.y1 << "]";
			return os;
		}
	};
}