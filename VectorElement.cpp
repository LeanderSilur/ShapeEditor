#include "VectorElement.h"

#include <cassert>
#include <iostream>
#include <algorithm>
#include <iterator>

#include <opencv2/imgproc.hpp>


namespace VE {

	void VectorElement::calcBounds(const std::vector<Point>& points)
	{
		float x0 = FMAX,
			y0 = FMAX,
			x1 = -FMAX,
			y1 = -FMAX;

		for (auto pt = points.begin(); pt != points.end(); pt++) {
			x0 = std::min(pt->x, x0);
			y0 = std::min(pt->y, y0);
			x1 = std::max(pt->x, x1);
			y1 = std::max(pt->y, y1);
		}
		bounds = Bounds(x0, y0, x1, y1);
	}

	void VectorElement::drawBoundingBox(cv::Mat& img, Transform& t)
	{
		Bounds tBounds(bounds);

		t.apply(tBounds);
		tBounds.Pad(8);
		auto rect = tBounds.Rect();
		cv::rectangle(img, tBounds.Rect(), BOUNDS_COLOR, BOUNDS_LINETHICKNESS, BOUNDS_LINETYPE);
	}

	void VectorElement::SimplifyNth(std::vector<Point>& points, const float& maxDist2)
	{
		if (points.size() <= 2)
			return;

		std::vector<Point> result;
		SimplifyNth(points, result, maxDist2);
		points = std::move(result);
	}

	// Simplifies point list by removing points (next in line), which are
	// not far away enough.
	void VectorElement::SimplifyNth(const std::vector<Point>& points, std::vector<Point>& result, const float& maxDist2)
	{
		if (points.size() <= 2)
			return;
		result.clear();
		std::vector<bool> mask(points.size());
		
		int j = 0;
		for (int i = 0; i < points.size(); )
		{
			mask[i] = true;
			Point pt = points[i];
			
			for (j = i + 1; j < points.size() - 1; j++) {
				Point diff = pt - points[j + 1];
				float dist2 = diff.x * diff.x + diff.y * diff.y;
				if (dist2 > maxDist2) {
					break;
				}
			}
			i = j;
		}

		mask.back() = true;

		for (int i = 0; i < mask.size(); ++i)
		{
			if (mask[i])
				result.push_back(points[i]);
		}
	}

	Bounds& VectorElement::getBounds()
	{
		return bounds;
	}
}