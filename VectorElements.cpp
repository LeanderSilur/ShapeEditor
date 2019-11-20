#include "VectorElements.h"

#include <cassert>
#include <iostream>
#include <algorithm>
#include <iterator>

#include <opencv2/imgproc.hpp>

#include "VectorElements.h"

namespace VE {
	int BEZIER_RESOLUTION = 12;
	int BEZIER_LINETYPE = cv::LINE_AA;
	int BEZIER_LINETHICKNESS = 1;
	cv::Scalar BEZIER_COLOR(80, 120, 200);
	double MIN_DRAWING_DISTANCE = 8.0;
	double MIN_DRAWING_DISTANCE2 = MIN_DRAWING_DISTANCE * MIN_DRAWING_DISTANCE;

	int POLYLINE_LINETYPE = cv::LINE_AA;
	int POLYLINE_LINETHICKNESS = 1;
	cv::Scalar POLYLINE_COLOR(200, 120, 80);

	cv::Scalar HIGHLIGHT_COLOR(255, 0, 0);
	cv::Scalar HIGHLIGHT_CIRCLE_COLOR(255, 200, 30);
	const int SEARCH_MAX_NEIGHBOURS = 10;
	const int SEARCH_FLANN_CHECKS = 12; // default 32



	inline bool LinesIntersect(Point & p, Point & p2, Point & q, Point & q2, Point & result)
	{
		assert((p != p2 && q != q2));


		if (p == q || p == q2) {
			result = p;
			return true;
		}
		else if (p2 == q || p2 == q2) {
			result = p2;
			return true;
		}

		VE::Point r = p2 - p;
		VE::Point s = q2 - q;
		VE::Point pq = q - p;

		double uNumerator = (pq).cross(r);
		double denominator = r.cross(s);

		// Vectors have no magnitude.
		if (fabs(uNumerator) < EPSILON && fabs(denominator) < EPSILON) {
			return false;
		}

		if (fabs(denominator) < EPSILON) {
			// lines are parallel
			// No shared endpoints, we already checked them at the start
			// of this function.
			return false;
		}

		double t = pq.cross(s) / denominator;
		double u = uNumerator / denominator;


		bool intersect = (t >= -EPSILON) && (t <= EPSILON_PLUS_ONE)
			&& (u >= -EPSILON) && (u <= EPSILON_PLUS_ONE);
		

		if (!intersect) return false;

		result = p + Point(r) * t;
		
		Point distance = p;
		if (p.x * p.x + p.y * p.y) {

		}

		std::cout << "((1-t)" << p.x << "+t " << p2.x << ",((1-t)" << p.y << "+t " << p2.y << "))\n" 
			<< "((1-t)" << q.x << "+t " << q2.x << ",((1-t)" << q.y << "+t " << q2.y << "))\n";
		std::cout << "  " << t << ", " << u << "\n";
		std::cin >> (std::string());

		return true;

	}


	void Polyline::calculateBounds()
	{
		Point start(FMAX, FMAX);
		Point end(-FMAX, -FMAX);

		for (auto pt = points.begin(); pt != points.end(); pt++) {
			start.x = std::min(pt->x, start.x);
			start.y = std::min(pt->y, start.y);
			end.x = std::max(pt->x, start.x);
			end.y = std::max(pt->y, start.y);
		}
	}

	double Polyline::distancePointLine2(Point & u, Point & v, Point & p, Point & result)
	{
		// |v-u|^2 
		const Point uv = v - u;
		Point closest = u;
		result = u;
		const double l2 = uv.x*uv.x + uv.y *uv.y;
		if (l2 != 0.0f) {
			const double t = (p - u).dot(uv) / l2;
			
			if (t >= 1){
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

		return closest.x*closest.x + closest.y*closest.y;
	}

	std::shared_ptr<Polyline> Polyline::splitOffAt(int & i, Point & intersection)
	{
		if (intersection == points.front() || intersection == points.back()) {
			return std::shared_ptr<Polyline>();
		}
		//std::cout << "Splitting " << points[i] << " .. " << points[i + 1] << "\n";

		std::shared_ptr<Polyline> newPtr = std::make_shared<Polyline>();

		// (1) Intersection is at 1st point.
		if (intersection == points[i]) {
			std::move(points.begin() + i, points.end(), std::back_inserter(newPtr->points));
			points.erase(points.begin() + i, points.end());
			points.push_back(intersection);
		}
		// (2) Intersection is at 2nd point.
		else if (intersection == points[i + 1]) {
			std::move(points.begin() + i + 1, points.end(), std::back_inserter(newPtr->points));
			points.erase(points.begin() + i + 1, points.end());
			points.push_back(intersection);
		}
		// (3) Intersection is inbetween points.
		else {
			newPtr->points.push_back(intersection);
			std::move(points.begin() + i + 1, points.end(), std::back_inserter(newPtr->points));
			points.erase(points.begin() + i + 1, points.end());
			// Duplicate double and cleanup self.
			points.push_back(intersection);
		}


		// Cleanup.
		Cleanup();
		newPtr->Cleanup();

		assert((points.size() >= 2 && newPtr->points.size() >= 2));

		return newPtr;
	}

	void Polyline::setPoints(std::vector<Point>& inputPoints)
	{
		this->points = inputPoints;
		Cleanup();
	}

	void Polyline::calculateKDTree()
	{
		// Clear the features matrix and the maximum length.

		//Use floats!
		features = cv::Mat_<float>(0, 2);
		double maxLength2 = 0.f;


		for (auto pt = points.begin(); pt != points.end(); pt++) {
			if (pt != points.begin()) {
				Point direction = *(pt - 1) - *pt;
				double length2 = direction.x * direction.x + direction.y * direction.y;
				maxLength2 = std::max(maxLength2, length2);
			}

			// Insert the point in the the KDTree.
			cv::Mat row = (cv::Mat_<float>(1, 2) << pt->x, pt->y);
			features.push_back(row);
		}
		maxLength = std::sqrt(maxLength2);
		flannIndex = std::make_shared<cv::flann::Index>(features, cv::flann::KDTreeIndexParams());
	}


	std::shared_ptr<Polyline> Polyline::splitIntersecting(Polyline & other)
	{
		// Check if the bounds don't overlap.
		if (bounds.x > other.bounds.x + other.bounds.width
			&& bounds.x + bounds.width < other.bounds.x
			&& bounds.y > other.bounds.y + other.bounds.height
			&& bounds.y + bounds.height < other.bounds.y) {
			return std::shared_ptr<Polyline>();
		}

		// See Closest() for more info.
		// The maximum distance is simply half of the maximum length of both Polylines
		// added together.
		double maxDistance = maxLength * maxLength / 4 + other.maxLength * other.maxLength + EPSILON;


		for (int i = 0; i < points.size() - 1; i++)
		{
			// Center position of segment.
			cv::Mat query = (cv::Mat_<double>(1, 2) << features[i][0], features[i][1]);
			cv::Mat indices, dists;
			other.flannIndex->radiusSearch(query, indices, dists, maxDistance, SEARCH_MAX_NEIGHBOURS,
				cv::flann::SearchParams(SEARCH_FLANN_CHECKS));

			// Compare each result from the others' flann search.
			for (int j = 0; j < SEARCH_MAX_NEIGHBOURS; j++) {
				int index = indices.at<int>(j);
				if (index < 0) {
					// No points found.
					j = SEARCH_MAX_NEIGHBOURS;
				}
				else {
					// If there is a valid segment nearby, compare its points to the 
					// current (i, i + 1) points.
					Point intersection;

					// Check if the segments intersect.
					bool doIntersect = LinesIntersect(points[i], points[i + 1],
						other.points[index], other.points[index + 1], intersection);

					if (doIntersect) {
						return splitOffAt(i, intersection);
					}
				}
			}

		}

		return std::shared_ptr<Polyline>();
	}

	Polyline::Polyline(std::vector<Point>& points)
	{
		setPoints(points);
	}

	void Polyline::PrependMove(Polyline & other, bool fromBackPoint)
	{
		if (fromBackPoint) {
			points.insert(points.begin(), std::make_move_iterator(other.points.begin()),
				std::make_move_iterator(other.points.end()));
		}
		else {
			points.insert(points.begin(), std::make_move_iterator(other.points.rbegin()),
				std::make_move_iterator(other.points.rend()));
		}
		other.points.clear();
		Cleanup();
	}

	void Polyline::AppendMove(Polyline & other, bool fromBackPoint)
	{
		if (fromBackPoint) {
			points.insert(points.end(), std::make_move_iterator(other.points.rbegin()),
				std::make_move_iterator(other.points.rend()));
		}
		else {
			points.insert(points.end(), std::make_move_iterator(other.points.begin()),
				std::make_move_iterator(other.points.end()));
		}
		other.points.clear();
		Cleanup();
	}

	void Polyline::Draw(cv::Mat & mat)
	{
		if (points.size() < 2) return;
		auto point = points.begin() + 1;

		while (point != points.end()) {
			cv::line(mat, *(point - 1), *point, POLYLINE_COLOR, POLYLINE_LINETHICKNESS, POLYLINE_LINETYPE);
			point++;
		}
	}

	void Polyline::Draw(cv::Mat & img, Transform2D & t, bool highlight)
	{
		cv::Scalar* color = &POLYLINE_COLOR;
		if (highlight)
			color = &HIGHLIGHT_COLOR;

		// determine the minimum distance between two points
		double minDist = VE::MIN_DRAWING_DISTANCE;
		VE::transformInv(minDist, t);
		double minDist2 = minDist * minDist;

		if (points.size() < 2) return;
		const std::vector<cv::Point2d> & points = this->points;

		// Only connect those with a minimum distance.
		auto point_it = points.begin();
		auto _pointA = point_it;
		Point pointA = *point_it;
		
		VE::transform(pointA, t);
		point_it++;

		while (point_it != points.end()) {
			Point pointB = *point_it;
			Point dir = pointB - *_pointA;
			// connect the points if they are greater than a certain distance
			if (dir.x * dir.x + dir.y * dir.y > minDist2) {
				VE::transform(pointB, t);
				cv::line(img, pointA, pointB, *color, POLYLINE_LINETHICKNESS, POLYLINE_LINETYPE);

				if (highlight)
					cv::circle(img, pointA, 1, HIGHLIGHT_CIRCLE_COLOR, 2);

				pointA = pointB;
				_pointA = point_it;
			}
			point_it++;
		}

		// Check if the last point was connected.
		Point pointB = *(point_it - 1);
		VE::transform(pointB, t);
		if (_pointA + 1 != points.end()) {
			cv::line(img, pointA, pointB, *color, POLYLINE_LINETHICKNESS, POLYLINE_LINETYPE);
		}
		if (highlight)
			cv::circle(img, pointB, 1, HIGHLIGHT_CIRCLE_COLOR, 2);

	}


	bool Polyline::InRect(cv::Rect2f & rect)
	{
		// Simple implementation, just checking if a point is in the rect.
		for (auto point = points.begin(); point != points.end(); point++)
		{
			if (rect.contains(*point))
				return true;
		}
		return false;
	}

	double Polyline::Distance2(Point & pt)
	{
		Point closest;
		double distance = FMAX;
		Closest2(pt, distance, closest);
		return distance;
	}

	void Polyline::Closest2(Point & pt, double & distance2, Point & closest)
	{
		// The distance2 parameter is the squared maximum distance.
		// cv::flann uses the squared distance, but I need to add
		// maxLength/2 ^2, because the features LUT only saved the centerpoints of line
		// segments.
		double maxDistance = distance2 + maxLength * maxLength / 4 + EPSILON;

		Point pointOnLine;

		cv::Mat query = (cv::Mat_<float>(1, 2) << pt.x, pt.y);
		cv::Mat indices, dists;
		flannIndex->radiusSearch(query, indices, dists, maxDistance, SEARCH_MAX_NEIGHBOURS,
			cv::flann::SearchParams(SEARCH_FLANN_CHECKS));
		

		// After potential segments have been found, check them against
		// the point to line distance, which will usually be much shorter, than the
		// distance between the line's center and the pt.
		for (int i = 0; i < SEARCH_MAX_NEIGHBOURS; i++) {
			int index = indices.at<int>(i);
			if (index < 0) {
				// No points found. 
				break;
			}

			double distanceOther = distancePointLine2(points[index], points[index + 1], pt, pointOnLine);
			if (distanceOther < distance2) {
				distance2 = distanceOther;
				closest = pointOnLine;
			}
		}
	}

	bool Polyline::Removedoubles()
	{
		if (points.size() < 2) return false;
		size_t initialSize = points.size();

		auto pt = points.begin() + 1;
		Point distance;
		while (pt != points.end()) {
			if (*(pt - 1) == *pt) {
				pt--;
				points.erase(pt+1);
			}
			pt++;
		}
		bool changed = initialSize != points.size();

		// This should be done somewhere else manually.
		/*
		if (changed) {
			calculateBounds();
			calculateKDTree();
		}*/
		return changed;
	}




	void Bezier::CalculateCoefficients()
	{
		coefficients[0] = -points[0] + 3 * points[1] - 3 * points[2] + points[3];
		coefficients[1] = 3 * points[0] - 6 * points[1] + 3 * points[2];
		coefficients[2] = -3 * points[0] + 3 * points[1];
		coefficients[3] = points[0];
	}

	void Bezier::calculateBounds()
	{
		Point start(FMAX, FMAX);
		Point end(-FMAX, -FMAX);

		for (int i = 0; i < 4; i++) {
			start.x = std::min(points[i].x, start.x);
			start.y = std::min(points[i].y, start.y);
			end.x = std::max(points[i].x, start.x);
			end.y = std::max(points[i].y, start.y);
		}
	}


	Bezier::Bezier()
	{
		std::fill_n(points, 4, cv::Point2i());
		CalculateCoefficients();
		calculateBounds();
	}

	Bezier::Bezier(Bezier & other)
	{
		for (int i = 0; i < 4; i++)
			points[i] = other.points[i];
		CalculateCoefficients();
		calculateBounds();
	}

	Bezier::Bezier(Point points[4])
	{
		setPoints(points);
	}

	Bezier::Bezier(Point & a, Point & b, Point & c, Point & d)
	{
		points[0] = a;
		points[1] = b;
		points[2] = c;
		points[3] = d;
		CalculateCoefficients();
		calculateBounds();
	}

	Bezier::Bezier(double ax, double ay, double bx, double by, double cx, double cy, double dx, double dy)
	{
		points[0] = Point(ax, ay);
		points[1] = Point(bx, by);
		points[2] = Point(cx, cy);
		points[3] = Point(dx, dy);
		CalculateCoefficients();
		calculateBounds();
	}

	void Bezier::setPoints(Point points[4])
	{
		for (int i = 0; i < 4; i++)
			this->points[i] = points[i];
		CalculateCoefficients();
		calculateBounds();
	}

	Point Bezier::At(double t)
	{
		return coefficients[0] * t*t*t + coefficients[1] * t*t + coefficients[2] * t + coefficients[3];
	}

	void Bezier::Draw(cv::Mat & mat)
	{
		double t = 0;
		Point pt0 = points[0];

		for (t = 1; t < BEZIER_RESOLUTION; t++)
		{
			Point pt1 = At(t / BEZIER_RESOLUTION);
			cv::line(mat, pt0, pt1, BEZIER_COLOR, BEZIER_LINETHICKNESS, BEZIER_LINETYPE);
			pt0 = pt1;
		}

		cv::line(mat, pt0, points[3], BEZIER_COLOR, BEZIER_LINETHICKNESS, BEZIER_LINETYPE);
	}

	void Bezier::Draw(cv::Mat & img, Transform2D & t, bool highlight)
	{
		cv::Scalar* color = &POLYLINE_COLOR;
		if (highlight)
			color = &HIGHLIGHT_COLOR;

		Bezier transformed(*this);
		for (int i = 0; i < 4; i++)
			VE::transform(transformed.points[i], t);

		transformed.CalculateCoefficients();
		transformed.Draw(img);
	}

	bool Bezier::InRect(cv::Rect2f & rect)
	{
		for (int i = 0; i < 4; i++)
		{
			if (rect.contains(points[i]))
				return true;
		}
		return false;
	}
	double Bezier::Distance2(Point & pt)
	{
		return 0;
	}
	void Bezier::Closest2(Point & pt, double & distance, Point & closest)
	{
	}


}