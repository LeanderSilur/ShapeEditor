#include "Polyline.h"
#include "opencv2/imgproc.hpp"
#include <iostream>

namespace VE {

	Polyline::LineStat Polyline::getStatus()
	{
		// check loop
		if (Front() == Back())
			return LineStat::loop;

		// check if connected to nothing
		if (ConnectFront.size() == 0 ||
			ConnectBack.size() == 0) {
			return LineStat::invalid;
		}
		// check if connected only to loop
		if (ConnectFront.size() == 2) {
			if (ConnectFront[0].polyline == ConnectFront[1].polyline)
				return LineStat::invalid;
		}
		if (ConnectBack.size() == 2) {
			if (ConnectBack[0].polyline == ConnectBack[1].polyline)
				return LineStat::invalid;
		}

		return LineStat::std;
	}

	int Polyline::FlannLookupSingle(const VE::Point &pt, const float& maxDist2)
	{
		return tree.nearest(pt, float(maxDist2));
	}

	void Polyline::setPoints(const std::vector<Point>& inputPoints)
	{
		this->points = inputPoints;
		Cleanup();
	}

	const std::vector<Point>& Polyline::getPoints()
	{
		return this->points;
	}

	const std::vector<Point>& Polyline::getSimplified(float maxDist2)
	{
		if (simple_maxDist2 != maxDist2) {
			simple_maxDist2 = maxDist2;
			UpdateSimplifiedPoints();
		}
		return simplifiedPoints;
	}

	void Polyline::UpdateSimplifiedPoints()
	{
		simplifiedPoints = points;
		SimplifyNth(simplifiedPoints, simple_maxDist2);
	}

	void Polyline::calculateKDTree()
	{
		// Calculate the maximum length and set the points.
		float maxLength2 = 0.f;

		for (auto pt = points.begin() + 1; pt != points.end(); pt++) {
			Point direction = *(pt - 1) - *pt;
			maxLength2 = std::max(maxLength2, direction.x * direction.x + direction.y * direction.y);
		}

		maxLength = std::sqrt(maxLength2);
		tree.setPoints(points);
	}


	Polyline::Polyline()
	{
		std::exception("Default Constructor shouldnt be used.");
	}

	Polyline::Polyline(const std::vector<Point>& points)
	{
		setPoints(points);
	}

	Polyline::~Polyline()
	{
	}


	void Polyline::Simplify(const float& maxDist)
	{
		SimplifyNth(points, maxDist * maxDist);
		simple_maxDist2 = maxDist * maxDist;
		simplifiedPoints = points;
	}

	inline float Mag(const Point& pt)
	{
		return std::sqrt(pt.x * pt.x + pt.y * pt.y);
	}
	void Polyline::Smooth(const int& iterations, const float& lambda)
	{
		simple_maxDist2 = -1;

		for (int i = 0; i < iterations; ++i)
		{
			for (int j = 1; j < (int)points.size() - 1; j++)
			{
				Point& prev = points[j - 1] - points[j];
				Point& next = points[j + 1] - points[j];
				float nPrev = Mag(prev),
					nNext = Mag(next);
				float wPrev = 1 / nPrev,
					wNext = 1 / nNext;

				Point L = (wPrev * prev + wNext * next) / (wPrev + wNext);
				points[j] += lambda * L;
			}
		}
	}

	void Polyline::Draw(cv::Mat& img, Transform& t, const cv::Scalar * colorOverride, bool circles)
	{
		// If this object has not loaded yet, don't draw anything.
		if (this == nullptr)
			return;

		cv::Scalar color;
		if (status == LineStat::std)
			color = POLYLINE_COLOR_STD;
		else if (status == LineStat::loop)
			color = POLYLINE_COLOR_LOOP;
		else if (status == LineStat::invalid)
			color = POLYLINE_COLOR_INVALID;

		float thickness = POLYLINE_LINETHICKNESS;
		if (colorOverride != nullptr) {
			color = *colorOverride;
			thickness = POLYLINE_LINETHICKNESS_HIGHLIGHT;
		}

		// determine the minimum distance between two points
		float minDist = MIN_DRAWING_DISTANCE;
		t.applyInv(minDist);
		float minDist2 = minDist * minDist;

		// Create a copy, then simplify it according to the Transforms mindist2.
		auto drawPoints = getSimplified(minDist2);

		for(auto&pt: drawPoints)
			t.apply(pt);

		if (drawPoints.empty())
			throw std::invalid_argument("Points are empty.");

		std::vector<cv::Point2i> tmp;
		cv::Mat(drawPoints).copyTo(tmp);

		const cv::Point* pts = (const cv::Point*) cv::Mat(tmp).data;
		int npts = cv::Mat(tmp).rows;

		cv::polylines(img, &pts, &npts, 1, false, color, thickness, POLYLINE_LINETYPE, 0);

		// End and highlighting.
		if (!circles) {
			cv::circle(img, tmp.front(), 1, POLYLINE_COLOR_ENDS, 2);
			cv::circle(img, tmp.back(), 1, POLYLINE_COLOR_ENDS, 2);
		}

		if (circles) {
			for (auto&pt:tmp)
			{
				cv::circle(img, pt, 1, HIGHLIGHT_CIRCLE_COLOR, 2);
			}
		}
		if (colorOverride != nullptr) {
			drawBoundingBox(img, t);
		}
	}

	bool Polyline::AnyPointInRect(const VE::Bounds & other)
	{
		if (!other.Overlap(bounds))
			return false;

		for (auto&pt:points)
			if (other.Contains(pt)) return true;

		return false;
	}

	float Polyline::Distance2(Point& pt)
	{
		Point closest;
		float distance2 = FMAX;
		int index = ClosestPt2(pt, distance2);

		return distance2;
	}

	// Returns the closest point.
	int Polyline::ClosestPt2(const Point& target, float& distance2)
	{
		return tree.nearest(target, distance2);
	}

	// Returns the point closest to the closest point on the line.
	int Polyline::ClosestLinePt2(const Point& target, float& distance2, VE::Point& ptOnLine, float& at)
	{
		float maxDist2 = distance2 + maxLength * maxLength / 4 + EPSILON;

		int idx = tree.nearest(target, maxDist2);
		if (idx < 0) return -1;

		int closestIdx = -1;

		if (idx < points.size() - 1) {
			if (points[idx] == points[idx + 1])
				throw std::exception("Points are the same.");

			const Point uv = points[idx + 1] - points[idx];
			const float mag2 = Magnitude2(uv);
			float t = (target - points[idx]).dot(uv) / mag2;
			t = t < 0 ? 0.f : t > 1 ? 1 : t;
			Point ptOnLineNext = points[idx] + t * uv;
			float d2 = VE::Distance2(ptOnLineNext, target);
			if (d2 < distance2) {
				closestIdx = idx;
				distance2 = d2;
				at = t;
				ptOnLine = ptOnLineNext;
			}
		}
		if (idx > 0) {
			if (points[idx] == points[idx - 1])
				throw std::exception("Points are the same.");

			const Point uv = points[idx - 1] - points[idx];
			const float mag2 = Magnitude2(uv);
			float t = (target - points[idx]).dot(uv) / mag2;
			t = t < 0 ? 0.f : t > 1 ? 1 : t;
			Point ptOnLineNext = points[idx] + t * uv;
			float d2 = VE::Distance2(ptOnLineNext, target);
			if (d2 < distance2) {
				closestIdx = idx;
				distance2 = d2;
				at = -t;
				ptOnLine = ptOnLineNext;
			}
		}
		return closestIdx;
	}

	// Get the Index of the point closest to (param) pt
	int Polyline::PointIndex(const Point& pt, const float& maxDist2)
	{
		Bounds dBounds(bounds);
		dBounds.Pad(std::sqrt(maxDist2));

		if (!dBounds.Contains(pt))
			return -1;

		// Use a kd lookup.
		float dist2 = maxDist2;
		return tree.nearest(pt, dist2);
	}

	bool Polyline::removeDoubles()
	{
		
		if (points.size() < 2) return false;
		size_t initialSize = points.size();

		auto pt = points.begin() + 1;
		Point distance;
		while (pt != points.end()) {
			if (*(pt - 1) == *pt) {
				pt--;
				points.erase(pt + 1);
			}
			pt++;
		}
		bool changed = initialSize != points.size();

		return changed;
	}


}