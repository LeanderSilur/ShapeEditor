#pragma once
#include <limits>
#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace VE {

	// big and small numbers
	const float FMAX = std::numeric_limits<float>::max();
	const float EPSILON = FLT_EPSILON ;
	const float EPSILON_PLUS_ONE = 1 + EPSILON;


	// B
	const float MIN_DRAWING_DISTANCE = 8.0;
	//const float MIN_DRAWING_AREA = 2.0;
	const float MIN_DRAWING_DISTANCE2 = MIN_DRAWING_DISTANCE * MIN_DRAWING_DISTANCE;

	// Polyline drawing options
	const int POLYLINE_LINETYPE = cv::LINE_AA;
	const int POLYLINE_LINETHICKNESS = 1.5;
	const int POLYLINE_LINETHICKNESS_HIGHLIGHT = 3;
	const cv::Scalar POLYLINE_COLOR_STD(200, 0, 200);
	const cv::Scalar POLYLINE_COLOR_LOOP(0, 200, 0);
	const cv::Scalar POLYLINE_COLOR_INVALID(255, 0, 0);
	const cv::Scalar POLYLINE_COLOR_ENDS(40, 40, 40);


	// Bounds drawing options
	const int BOUNDS_LINETYPE = cv::LINE_4;
	const int BOUNDS_LINETHICKNESS = 1;
	const cv::Scalar BOUNDS_COLOR(255, 0, 255);


	// Highlighting
	const cv::Scalar HIGHLIGHT_COLOR(255, 0, 0);
	const cv::Scalar HIGHLIGHT_CIRCLE_COLOR(170, 150, 30);


	// typedefs
	typedef cv::Point2f Point;
	class Polyline;
	class Polyshape;
	class PolyshapeData;
	class ColorArea;
	typedef std::shared_ptr<Polyline> PolylinePtr;
	typedef std::shared_ptr<Polyshape> PolyshapePtr;
	typedef std::shared_ptr<ColorArea> ColorAreaPtr;

	static inline float Distance(const Point& a, const Point& b) {
		float x = b.x - a.x;
		float y = b.y - a.y;
		return std::sqrt(x * x + y * y);
	};
	static inline float Distance2(const Point& a, const Point& b) {
		float x = b.x - a.x;
		float y = b.y - a.y;
		return x * x + y * y;
	};
	static inline float Magnitude(const Point& v) {
		return std::sqrt(v.x * v.x + v.y * v.y);
	};
	static inline float Magnitude2(const Point& v) {
		return v.x * v.x + v.y * v.y;
	};

	static inline float Cross(const VE::Point& a, const VE::Point& b) {
		return a.x * b.y - a.y * b.x;
	}
	static inline float Dot(const VE::Point& a, const VE::Point& b) {
		return a.x * b.x + a.y * b.y;
	}

	static inline float DistancePointLineParam(const Point& u, const Point& v, const Point& pt)
	{
		const Point uv = v - u;
		const float mag2 = Magnitude2(uv);
		if (mag2 != 0.0f)
			return (pt - u).dot(uv) / mag2;
		else
			throw std::exception("Vector has no length.");
	}

	static inline float DistancePointLineParam(const Point& u, const Point& uv, const float& magUV, const Point& pt)
	{
		return (pt - u).dot(uv) / magUV;
	}

	// Returns the closest point between u and v.
	static inline float DistancePointLine2(const Point& u, const Point& v, const Point& pt, Point& result)
	{
		const Point uv = v - u;
		const float mag2 = Magnitude2(uv);

		float t = 0.f;
		if (mag2 != 0.0f)
			t = (pt - u).dot(uv) / mag2;

		result = u + t * uv;
		return Magnitude2(result - pt);
	}

	static inline float DistancePointLineClip2(const Point& u, const Point& v, const Point& pt, Point& result, float& t)
	{
		const Point uv = v - u;
		const float mag2 = Magnitude2(uv);

		t = 0.f;
		if (mag2 != 0.0f)
			t = (pt - u).dot(uv) / mag2;
		if (t < 0) t = 0;
		if (t > 1) t = 1;
		result = u + t * uv;
		return Magnitude2(result - pt);
	}

	static inline float DistancePointLineClip2(const Point& u, const Point& v, const Point& pt, Point& result)
	{
		float t;
		VE::DistancePointLineClip2(u, v, pt, result, t);
	}

	// Snaps the result point to either u or v.
	static inline float DistancePointLineSnapped2(const Point& u, const Point& v, const Point& pt, Point& result)
	{
		// |v-u|^2 
		const Point uv = v - u;
		Point closest = u;
		result = u;
		const float mag2 = Magnitude2(uv);
		if (mag2 != 0.0f) {
			const float t = (pt - u).dot(uv) / mag2;

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

		closest -= pt;
		return closest.x * closest.x + closest.y * closest.y;
	}
}