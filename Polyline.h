#pragma once

#include "VectorElement.h"
#include "opencv2/flann/miniflann.hpp"

namespace VE {

	class Polyshape;

	class Polyline : public VectorElement{
	public:
		// connectivity status
		enum class LineStat { std, loop, invalid };
	protected:
		std::vector<Point> points;

		cv::Mat_<float> features;
		std::shared_ptr<cv::flann::Index> flannIndex;
		float maxLength;

		Polyline::LineStat status = LineStat::std;

		bool removeDoubles();
		void calculateKDTree();
		float distancePointLine2(const Point& u, const Point& v, const Point& p, Point& result);

		LineStat getStatus();
		int FlannLookupSingle(const Point &pt, const float& maxDist2);
	public:

		void Cleanup() {
			removeDoubles();
			calcBounds(points);
			calculateKDTree();
		};

		void setPoints(std::vector<Point>& inputPoints);
		Point& getPoint(const int& i) { return points[i]; };
		std::vector<Point>& getPoints();

		Polyline();
		Polyline(std::vector<Point>& points);
		~Polyline();

		Point& Front() { return points.front(); };
		Point& Back() { return points.back(); };
		Point& Front1() { return points[1]; };
		Point& Back1() { return points[points.size() - 2]; };

		std::vector<Connection> ConnectFront;
		std::vector<Connection> ConnectBack;

		void Simplify(const float& maxDist);
		void Smooth(const int& iterations = 10, const float& lambda = 0.5);

		void UpdateStatus() { status = getStatus(); };
		LineStat Status() { return status; };

		void Draw(cv::Mat& img, Transform& t, bool highlight = false);

		bool AnyPointInRect(VE::Bounds& bounds);
		float Distance2(Point& pt);
		void Closest2(
			const Point& pt,
			float& distance2,
			Point& closest);
		Bounds& getBounds();

		int PointIndex(const Point& pt, const float& maxDist2 = 0);
		bool LongEnough() { return points.size() >= 2; };
		size_t Length() { return points.size(); };

	};
}