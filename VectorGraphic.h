#pragma once

#include <vector>
#include <opencv2/core.hpp>

#include "VectorElements.h"


typedef std::shared_ptr<VE::Polyline> PolylinePointer;
typedef struct Connection {
public:
	enum Location { none, start, end };
	PolylinePointer polyline;
	Location at;

	void Invert() {
		if (at == Location::start) at == Location::end;
		else if (at == Location::end) at == Location::start;
	};
};
class VectorGraphic {
private:
	// Used in SnapEndpoints(). The distance2 of an endpoint to a
	// potential intersection point.
	const float SNAPPING_DISTANCE2 = 0.2;
	// Used in MergeConnected(). If two linesegments share an enpoint
	// they can be connected if this angle isn't exceeded.
	const float MAX_MERGE_ANGLE = CV_PI/16;


	// returns the closest point on the polyline
	bool closestPointInRange(const VE::Point & center,
		std::vector<PolylinePointer> Polylines,
		VE::Point& result, float maxDist2);
	
	std::vector<Connection> GetConnections(const VE::Point& pt, const std::vector<PolylinePointer>& polylines);
public:

	VectorGraphic() {};
	

	std::vector<PolylinePointer> Polylines;

	void LoadPolylines(std::string path);

	void SnapEndpoints();
	void RemoveOverlaps();
	void MergeConnected();

	// bloated complicated method, used for the imageviewer
	void ClosestElement(cv::Mat& img, VE::Transform2D& t, float& distance, const VE::Point& pt,
		VE::Point& closest, std::shared_ptr<VE::VectorElement>& element);

	void Draw(cv::Mat & img);
	void Draw(cv::Mat & img, VE::Transform2D & t);
};