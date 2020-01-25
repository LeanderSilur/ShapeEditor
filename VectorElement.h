#pragma once

#include "Constants.h"
#include "Bounds.h"
#include "Transform.h"
#include "Connection.h"
#include "ColorArea.h"


namespace cv {
	class Mat;
}

namespace VE {
	class VectorElement {
	protected:
		Bounds bounds;
		
		void calcBounds(const std::vector<Point> & points);
		void drawBoundingBox(cv::Mat& img, Transform& t);
		void SimplifyNth(std::vector<Point>& points, const float& maxDist2);
		void SimplifyNth(const std::vector<Point>& points, std::vector<Point>& result, const float& maxDist2);

	public:
		Bounds& getBounds();
	};
}