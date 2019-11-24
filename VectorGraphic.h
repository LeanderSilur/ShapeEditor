#pragma once

#include <vector>
#include "VectorElements.h"

typedef std::shared_ptr<VE::Polyline> PolylinePointer;
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
		std::vector<PolylinePointer> Polylines,
		VE::Point& result, float maxDist2);
	
	std::vector<VE::Connection> GetConnections(const VE::Point& pt, const std::vector<PolylinePointer>& polylines);
	inline float GetConnectionAngle(const VE::Connection& conA, const VE::Connection& conB);
public:

	VectorGraphic() {};
	

	std::vector<PolylinePointer> Polylines;

	void LoadPolylines(std::string path);

	void SnapEndpoints();
	void RemoveOverlaps();
	void MergeConnected();
	void ComputeConnections();

	// bloated complicated method, used for the imageviewer
	void ClosestElement(cv::Mat& img, VE::Transform2D& t, float& distance, const VE::Point& pt,
		VE::Point& closest, std::shared_ptr<VE::VectorElement>& element);

	void Draw(cv::Mat & img);
	void Draw(cv::Mat & img, VE::Transform2D & t);
};