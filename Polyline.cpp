#include "Polyline.h"
#include "opencv2/imgproc.hpp"
#include <iostream>

namespace VE {

	float Polyline::distancePointLine2(const Point& u, const Point& v, const Point& p, Point& result)
	{
		// |v-u|^2 
		const Point uv = v - u;
		Point closest = u;
		result = u;
		const float l2 = uv.x * uv.x + uv.y * uv.y;
		if (l2 != 0.0f) {
			const float t = (p - u).dot(uv) / l2;

			if (t >= 1) {
				closest = v;
				result = v;
			}
			else {
				if (t >= 0.5f)
					result = v;
				if (t >= 0.f)
					closest = u + t * uv;
			}
		}

		closest -= p;

		return closest.x * closest.x + closest.y * closest.y;
	}

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
		cv::Mat query = (cv::Mat_<float>(1, 2) << pt.x, pt.y);
		cv::Mat indices, dists;
		flannIndex->radiusSearch(query, indices, dists, maxDist2, 1, cv::flann::SearchParams(32));

		return indices.at<int>(0);
	}

	void Polyline::setPoints(std::vector<Point>& inputPoints)
	{
		this->points = inputPoints;
		Cleanup();
	}

	std::vector<Point>& Polyline::getPoints()
	{
		return this->points;
	}

	void Polyline::calculateKDTree()
	{
		// Clear the features matrix and the maximum length.
		//Use floats!
		features = cv::Mat_<float>(0, 2);
		float maxLength2 = 0.f;


		for (auto pt = points.begin(); pt != points.end(); pt++) {
			if (pt != points.begin()) {
				Point direction = *(pt - 1) - *pt;
				float length2 = direction.x * direction.x + direction.y * direction.y;
				maxLength2 = std::max(maxLength2, length2);
			}

			// Insert the point in the the KDTree.
			cv::Mat row = (cv::Mat_<float>(1, 2) << pt->x, pt->y);
			features.push_back(row);
		}

		maxLength = std::sqrt(maxLength2);
		flannIndex = std::make_shared<cv::flann::Index>(features, cv::flann::KDTreeIndexParams());
	}


	Polyline::Polyline()
	{
	}

	Polyline::Polyline(std::vector<Point>& points)
	{
		setPoints(points);
	}

	Polyline::~Polyline()
	{
	}


	void Polyline::Simplify(const float& maxDist)
	{
		SimplifyNth(points, maxDist * maxDist);
	}

	inline float Mag(const Point& pt)
	{
		return std::sqrt(pt.x * pt.x + pt.y * pt.y);
	}
	void Polyline::Smooth(const int& iterations, const float& lambda)
	{
		for (int i = 0; i < iterations; ++i)
		{
			for (int j = 1; j < points.size() - 1; j++)
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

	void Polyline::Draw(cv::Mat& img, Transform& t, cv::Scalar* pcolor, bool circles)
	{
		cv::Scalar color;
		if (status == LineStat::std)
			color = POLYLINE_COLOR_STD;
		else if (status == LineStat::loop)
			color = POLYLINE_COLOR_LOOP;
		else if (status == LineStat::invalid)
			color = POLYLINE_COLOR_INVALID;

		if (pcolor != nullptr)
			color = *pcolor;

		// determine the minimum distance between two points
		float minDist = MIN_DRAWING_DISTANCE;
		t.applyInv(minDist);
		float minDist2 = minDist * minDist;


		// Create a copy, then simplify it according to the Transforms mindist2.
		auto drawPoints = points;
		SimplifyNth(drawPoints, minDist2);

		for(auto&pt: drawPoints)
			t.apply(pt);

		std::vector<cv::Point2i> tmp;
		cv::Mat(drawPoints).copyTo(tmp);

		const cv::Point* pts = (const cv::Point*) cv::Mat(tmp).data;
		int npts = cv::Mat(tmp).rows;

		cv::polylines(img, &pts, &npts, 1, false, color, POLYLINE_LINETHICKNESS, POLYLINE_LINETYPE, 0);

		if (circles) {
			for (auto&pt:tmp)
			{
				cv::circle(img, pt, 1, HIGHLIGHT_CIRCLE_COLOR, 2);
			}
		}
		if (highlight) {
			drawBoundingBox(img, t);
		}
	}

	bool Polyline::AnyPointInRect(VE::Bounds& other)
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
		float distance = FMAX;
		Closest2(pt, distance, closest);
		return distance;
	}

	void Polyline::Closest2(const Point& from, float& distance2, Point& closest)
	{
		// The distance2 parameter is the squared maximum distance.
		// cv::flann uses the squared distance, but I need to add
		// maxLength/2  ^2
		// TRUST ME!!!! I've checked this twice. Even though the param name is called radius.
		float maxDistance = distance2 + maxLength * maxLength / 4 + EPSILON;

		cv::Mat query = (cv::Mat_<float>(1, 2) << from.x, from.y);
		cv::Mat indices, dists;
		flannIndex->radiusSearch(query, indices, dists, maxDistance, SEARCH_MAX_NEIGHBOURS,
			cv::flann::SearchParams(SEARCH_FLANN_CHECKS));


		// After potential segments have been found, check them against
		// the point to line distance, which will usually be shorter
		// than the distance between the vertex and the search_point.
		for (int i = 0; i < SEARCH_MAX_NEIGHBOURS; i++) {
			int index = indices.at<int>(i);
			if (index < 0) {
				// No (more) points found.
				break;
			}
			Point pointOnLine;
			float distancePrev = FMAX;
			float distanceNext = FMAX;
			if (index > 0) {
				distancePrev = distancePointLine2(points[index], points[index - 1], from, pointOnLine);
			}
			if (index < points.size() - 1) {
				distanceNext = distancePointLine2(points[index], points[index + 1], from, pointOnLine);
			}

			if (distance2 > std::min(distancePrev, distanceNext)) {
				Point other;
				if (distancePrev < distanceNext) {
					distance2 = distancePrev;
					other = points[index - 1];
				}
				else {
					distance2 = distanceNext;
					other = points[index + 1];
				}
				Point d1 = from - points[index];
				Point d2 = from - other;
				if (d1.x * d1.x + d1.y * d1.y < d2.x * d2.x + d2.y * d2.y)
					closest = points[index];
				else
					closest = other;
			}
		}
	}

	Bounds& Polyline::getBounds()
	{
		return bounds;
	}

	// Get the Index of the point closest to (param) pt
	int Polyline::PointIndex(const Point& pt, const float& maxDist2)
	{
		Bounds dBounds(bounds);
		dBounds.Pad(std::sqrt(maxDist2));

		if (!dBounds.Contains(pt))
			return -1;

		// The Flann Lookup seems to be much slower.
		//FlannLookupSingle();

		// If we are looking for a point at the exact position, we can use
		// an optimized search.
		if (maxDist2 == 0) {
			for (size_t i = 0; i < points.size(); i++)
			{
				if (points[i] == pt) {
					return i;
				}
			}
			return -1;
		}

		// If we need the closest point, then we have to find the one
		// with the smallest distance2.
		// This means we have to loop through all the points.
		float minDist2 = maxDist2;
		int minIndex = -1;

		for (int i = 0; i < points.size(); i++)
		{
			float x_ = pt.x - points[i].x;
			float y_ = pt.y - points[i].y;
			float dist2 = x_ * x_ + y_ * y_;
			if (dist2 < minDist2) {
				minDist2 = dist2;
				minIndex = i;
			}
		}
		return minIndex;
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