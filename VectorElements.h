#pragma once

#include <iostream>
#include <vector>
#include <limits>
#include <opencv2/core.hpp>
#include <opencv2/flann/miniflann.hpp>



namespace VE {

	const double DMAX = std::numeric_limits<double>::max();

	const double EPSILON = FLT_EPSILON;
	const double EPSILON_PLUS_ONE = 1 + EPSILON;
	const double SNAPPING_DISTANCE2 = 0.0000001f;

	typedef cv::Point2d Point;

	typedef struct Transform2D {
		double scale;
		double x;
		double y;
	};

	inline void transform(Point& pt, Transform2D& t) {
		pt *= t.scale;
		pt.x += t.x; pt.y += t.y;
	};
	inline void transformInv(Point& pt, Transform2D& t) {
		pt.x -= t.x; pt.y -= t.y;
		pt /= t.scale;
	};
	inline void transform(double& d, Transform2D& t) {
		d *= t.scale;
	};
	inline void transformInv(double& d, Transform2D& t) {
		d /= t.scale;
	};
	inline void transform(cv::Rect2d& rect, Transform2D& t) {
		rect.x = t.scale * rect.x + t.x;
		rect.y = t.scale * rect.y + t.y;
		rect.width *= t.scale;
		rect.height *= t.scale;
	};
	inline void transformInv(cv::Rect2d& rect, Transform2D& t) {
		rect.x = (rect.x - t.x) / t.scale;
		rect.y = (rect.y - t.y) / t.scale;
		rect.width /= t.scale;
		rect.height /= t.scale;
	};

	inline bool LinesIntersect(Point & p, Point & p2, Point & q, Point & q2, Point& result);
	

	// VectorElement class
	// baseclass for Polyline, Bezier
	class VectorElement {
	public:

		virtual void Draw(cv::Mat & img) = 0;
		virtual void Draw(cv::Mat & img, Transform2D & t, bool highlight = false) = 0;
		virtual bool AnyPointInRect(cv::Rect2d & rect) = 0;
		virtual double Distance2(Point & pt) = 0;
		virtual void Closest2(
			const Point & from,
			double& distance2, // init with std::numeric_limits<double>::max()
			Point & closest) = 0;
	};



	//Polyline
	class Polyline : public VectorElement {
	private:
		std::vector<Point> points;
		
		cv::Rect2d bounds;
		cv::Mat_<double> features;
		std::shared_ptr<cv::flann::Index> flannIndex;
		double maxLength;

		bool Removedoubles();
		void calculateKDTree();
		void calculateBounds();
		double distancePointLine2(const Point &u, const Point &v, const Point &p, Point&result);
		std::shared_ptr<Polyline> splitOffAt(int &at, Point&intersection);

	public:
		void Cleanup() {
			Removedoubles();
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
		bool AnyPointInRect(cv::Rect2d & rect) override;
		double Distance2(Point & pt) override;
		void Closest2(
			const Point& pt,
			double & distance2,
			Point & closest) override;
		int PointIndex(const Point& pt, const double& maxDist2 = 0);
		bool LongEnough() { return points.size() >=2; };
		size_t Length() { return points.size(); };

		Point FrontPoint() { return points[0]; }
		Point BackPoint() { return points[points.size() - 1]; }
	};





	// Cubic bezier.
	class Bezier : public VectorElement {

	private:
		Point points[4];
		Point coefficients[4];
		cv::Rect2d bounds;

		void CalculateCoefficients();
		void calculateBounds();

	public:


		Bezier();
		~Bezier() {};
		Bezier(Bezier & other);
		Bezier(Point points[4]);
		Bezier(Point & a, Point & b, Point & c, Point & d);
		Bezier(double ax, double ay, double bx, double by, double cx, double cy, double dx, double dy);

		void setPoints(Point points[4]);
		Point At(double t);


		void Draw(cv::Mat & img) override;
		void Draw(cv::Mat & img, Transform2D & t, bool highlight = false) override;
		bool AnyPointInRect(cv::Rect2d & rect) override;
		double Distance2(Point & pt) override;
		void Closest2(
			const Point& pt,
			double & distance,
			Point & closest) override;
	};
}