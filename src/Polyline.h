#pragma once

#include "VectorElement.h"
#include "PtTree.h"

namespace VE {

	class Polyshape;

	class Polyline : public VectorElement{
	public:
		// connectivity status
		enum class LineStat { std, loop, invalid };
	protected:
		std::vector<Point> points;
		std::vector<Point> simplifiedPoints;
		float simple_maxDist2 = -1;

		PtTree tree;
		float maxLength;

		Polyline::LineStat status = (LineStat)0;

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
			UpdateSimplifiedPoints();
		};

		void setPoints(std::vector<Point>& inputPoints);
		const Point& getPoint(const int& i) { return points[i]; };
		const std::vector<Point>& getPoints();

		const std::vector<Point>& getSimplified(float maxDist2);
	private:
		void UpdateSimplifiedPoints();
	public:
		inline float getMaxLength() { return maxLength; };

		Polyline();
		Polyline(std::vector<Point>& points);
		~Polyline();

		// First point in Polyline.
		const Point& Front() { return points.front(); };
		// Last point in Polyline.
		const Point& Back() { return points.back(); };
		// Second point in Polyline.
		const Point& Front1() { return points[1]; };
		// Penultimate point in Polyline.
		const Point& Back1() { return points[points.size() - 2]; };

		std::vector<Connection> ConnectFront;
		std::vector<Connection> ConnectBack;

		void Simplify(const float& maxDist);
		void Smooth(const int& iterations = 10, const float& lambda = 0.5);

		void UpdateStatus() { status = getStatus(); }
		void ResetStatus() { status = LineStat::std; };
		LineStat Status() { return status; };

		void Draw(cv::Mat& img, Transform& t, const cv::Scalar * colorOverride = nullptr, bool circles = false);

		bool AnyPointInRect(const VE::Bounds & other);
		float Distance2(Point& pt);
		int ClosestIdx2(
			const Point& pt,
			float& distance2,
			Point& closest);

		int PointIndex(const Point& pt, const float& maxDist2 = 0);
		bool LongEnough() { return points.size() >= 2; };
		size_t Length() { return points.size(); };

	};
}