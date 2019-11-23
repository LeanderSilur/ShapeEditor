#pragma once

#include <iostream>
#include <vector>
#include <limits>
#include <opencv2/core.hpp>
#include <opencv2/flann/miniflann.hpp>



namespace VE {

	const float FMAX = std::numeric_limits<float>::max();

	const float EPSILON = FLT_EPSILON;
	const float EPSILON_PLUS_ONE = 1 + EPSILON;
	const float SNAPPING_DISTANCE2 = 0.0000001f;

	typedef cv::Point2f Point;

	typedef struct Transform2D {
		float scale;
		float x, y;
	};
	typedef struct Bounds {
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
		}
		Bounds(const Bounds& other) {
			x0 = other.x0;
			y0 = other.y0;
			x1 = other.x1;
			y1 = other.y1;
		}
		bool Contains(const Point& pt) {
			return (pt.x >= x0 && pt.x <= x1 &&
				pt.y >= y0 && pt.y <= y1);
		}
		void Pad(const float& padding) {
			x0 -= padding; y0 -= padding;
			x1 += padding; y1 += padding;
		}
		cv::Rect2f Rect() {
			return cv::Rect2f(x0, y0, x1 - x0, y1 - y0);
		}
		friend std::ostream& operator<<(std::ostream& os, const Bounds& b)
		{
			os << "[" << b.x0 << ", " << b.y0 << " - " << b.x1 << ", " << b.y1 << "]";
			return os;
		}
	};

	inline void transform(Point& pt, const Transform2D& t) {
		pt *= t.scale;
		pt.x += t.x; pt.y += t.y;
	};
	inline void transformInv(Point& pt, const Transform2D& t) {
		pt.x -= t.x; pt.y -= t.y;
		pt /= t.scale;
	};
	inline void transform(float& d, const Transform2D& t) {
		d *= t.scale;
	};
	inline void transformInv(float& d, const Transform2D& t) {
		d /= t.scale;
	};
	inline void transform(Bounds& bounds, const Transform2D& t) {
		bounds.x0 *= t.scale; bounds.y0 *= t.scale;
		bounds.x1 *= t.scale; bounds.y1 *= t.scale;
		bounds.x0 += t.x; bounds.y0 += t.y;
		bounds.x1 += t.x; bounds.y1 += t.y;
	};
	inline void transformInv(Bounds& bounds, const Transform2D& t) {
		bounds.x0 -= t.x; bounds.y0 -= t.y;
		bounds.x1 -= t.x; bounds.y1 -= t.y;
		bounds.x0 /= t.scale; bounds.y0 /= t.scale;
		bounds.x1 /= t.scale; bounds.y1 /= t.scale;
	};

	inline bool LinesIntersect(Point & p, Point & p2, Point & q, Point & q2, Point& result);
	

	// VectorElement class
	// baseclass for Polyline, Bezier
	class VectorElement {
	public:

		virtual void Draw(cv::Mat & img) = 0;
		virtual void Draw(cv::Mat & img, Transform2D & t, bool highlight = false) = 0;
		virtual bool AnyPointInRect(cv::Rect2f & rect) = 0;
		virtual float Distance2(Point & pt) = 0;
		virtual void Closest2(
			const Point & from,
			float& distance2, // init with std::numeric_limits<float>::max()
			Point & closest) = 0;
	};



	//Polyline
	class Polyline : public VectorElement {
	private:
		std::vector<Point> points;
		
		Bounds bounds;
		cv::Mat_<float> features;
		std::shared_ptr<cv::flann::Index> flannIndex;
		float maxLength;

		bool RemoveDoubles();
		void calculateKDTree();
		void calculateBounds();
		float distancePointLine2(const Point &u, const Point &v, const Point &p, Point&result);
		std::shared_ptr<Polyline> splitOffAt(int &at, Point&intersection);

	public:
		void Cleanup() {
			RemoveDoubles();
			calculateBounds();
			calculateKDTree();
		};
		
		void setPoints(std::vector<Point>& inputPoints);
		Point& getPoint(const int& i) { return points[i]; };
		std::vector<Point>& getPoints();
		std::shared_ptr<Polyline> splitIntersecting(Polyline & other);

		Polyline();
		Polyline(std::vector<Point> & points);
		~Polyline();

		Point& Front() { return points.front(); };
		Point& Back() { return points.back(); };

		void PrependMove(Polyline & other, bool fromBackPoint);
		void AppendMove(Polyline & other, bool fromBackPoint);

		void Draw(cv::Mat & img) override;
		void Draw(cv::Mat & img, Transform2D & t, bool highlight = false) override;
		bool AnyPointInRect(cv::Rect2f & rect) override;
		float Distance2(Point & pt) override;
		void Closest2(
			const Point& pt,
			float & distance2,
			Point & closest) override;
		int PointIndex(const Point& pt, const float& maxDist2 = 0);
		bool LongEnough() { return points.size() >=2; };
		size_t Length() { return points.size(); };

	};





	// Cubic bezier.
	class Bezier : public VectorElement {

	private:
		Point points[4];
		Point coefficients[4];
		cv::Rect2f bounds;

		void CalculateCoefficients();
		void calculateBounds();

	public:


		Bezier();
		~Bezier() {};
		Bezier(Bezier & other);
		Bezier(Point points[4]);
		Bezier(Point & a, Point & b, Point & c, Point & d);
		Bezier(float ax, float ay, float bx, float by, float cx, float cy, float dx, float dy);

		void setPoints(Point points[4]);
		Point At(float t);


		void Draw(cv::Mat & img) override;
		void Draw(cv::Mat & img, Transform2D & t, bool highlight = false) override;
		bool AnyPointInRect(cv::Rect2f & rect) override;
		float Distance2(Point & pt) override;
		void Closest2(
			const Point& pt,
			float & distance,
			Point & closest) override;
	};
}