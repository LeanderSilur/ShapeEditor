#pragma once

#include <vector>
#include "VectorElement.h"

class VectorGraphic {
private:
	// Used in SnapEndpoints(). The distance2 of an endpoint to a
	// potential intersection point.
	const float SNAPPING_DISTANCE2 = 4;
	// Used in MergeConnected(). If two linesegments share an enpoint
	// they can be connected if this angle isn't exceeded.
	const float MIN_MERGE_ANGLE = CV_PI * 0.5;


	// returns the closest point on the polyline
	bool closestPointInRange(const VE::Point & center,
		std::vector<VE::PolylinePtr> Polylines,
		VE::Point& result, float maxDist2);
	
	std::vector<VE::Connection> GetConnections(const VE::Point& pt, const std::vector<VE::PolylinePtr>& polylines);
	inline float GetConnectionAngle(const VE::Connection& conA, const VE::Connection& conB);
public:

	VectorGraphic() {};
	

	std::vector<VE::PolylinePtr> Polylines;
	std::vector<VE::PolyshapePtr> Polyshapes;

	void LoadPolylines(std::string path);

	void SnapEndpoints();
	void RemoveOverlaps();
	void MergeConnected();
	void ComputeConnections();

	// bloated complicated method, used for the imageviewer
	void ClosestPolyline(cv::Mat& img, VE::Transform& t, float& distance2, const VE::Point& pt,
		VE::Point& closest, VE::PolylinePtr& element);

	void Draw(cv::Mat & img, VE::Transform& t);
};