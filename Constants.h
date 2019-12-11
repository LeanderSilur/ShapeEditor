#pragma once
#include <limits>
#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace VE {

	// big and small numbers
	const float FMAX = std::numeric_limits<float>::max();
	const float EPSILON = FLT_EPSILON;
	const float EPSILON_PLUS_ONE = 1 + EPSILON;


	// B
	const float MIN_DRAWING_DISTANCE = 8.0;
	//const float MIN_DRAWING_AREA = 2.0;
	const float MIN_DRAWING_DISTANCE2 = MIN_DRAWING_DISTANCE * MIN_DRAWING_DISTANCE;

	// Polyline drawing options
	const int POLYLINE_LINETYPE = cv::LINE_AA;
	const int POLYLINE_LINETHICKNESS = 1;
	const int POLYLINE_LINETHICKNESS_HIGHLIGHT = 3;
	const cv::Scalar POLYLINE_COLOR_STD(160, 24, 160);
	const cv::Scalar POLYLINE_COLOR_LOOP(0, 200, 0);
	const cv::Scalar POLYLINE_COLOR_INVALID(255, 0, 0);

	// Bounds drawing options
	const int BOUNDS_LINETYPE = cv::LINE_4;
	const int BOUNDS_LINETHICKNESS = 1;
	const cv::Scalar BOUNDS_COLOR(255, 0, 255);

	//
	const cv::Scalar HIGHLIGHT_COLOR(255, 0, 0);
	const cv::Scalar HIGHLIGHT_CIRCLE_COLOR(170, 150, 30);


	// Flannindex search options
	const int SEARCH_MAX_NEIGHBOURS = 10;
	const int SEARCH_FLANN_CHECKS = 64; // default 32


	// typedefs
	typedef cv::Point2f Point;
	class Polyline;
	class Polyshape;
	class PolyshapeData;
	class ColorArea;
	typedef std::shared_ptr<Polyline> PolylinePtr;
	typedef std::shared_ptr<Polyshape> PolyshapePtr;
	typedef std::shared_ptr<ColorArea> ColorAreaPtr;
}