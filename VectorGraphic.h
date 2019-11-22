#pragma once

#include <vector>
#include <opencv2/core.hpp>

#include "VectorElements.h"


typedef std::shared_ptr<VE::Polyline> PolylinePointer;
typedef struct ConnectionStart {
public:
	std::vector<std::shared_ptr<VE::Polyline>>::iterator polylineIt;
	bool fromBackPoint;
};
class VectorGraphic {

private:

	const double SNAPPING_DISTANCE2 = 0.1;
	std::vector<ConnectionStart> GetConnected(std::vector<PolylinePointer>::iterator startElement, VE::Point &pt);

	// returns the closest point on the polyline
	bool closestPointInRange(const VE::Point & center,
 std::vector<PolylinePointer> Polylines,
		VE::Point& result, double maxDist2);
public:

	VectorGraphic();
	VectorGraphic(const VectorGraphic& other);
	

	std::vector<PolylinePointer> Polylines;

	void LoadPolylines(std::string path);

	void SnapEndpoints();
	void RemoveOverlaps();
	void RemoveIntersections();
	void RemoveMaxLength(int length = 12);
	void MergeConnected();

	// bloated complicated method, used for the imageviewer
	void ClosestElement(cv::Mat& img, VE::Transform2D& t, double& distance, const VE::Point& pt,
		VE::Point& closest, std::shared_ptr<VE::VectorElement>& element);

	void Draw(cv::Mat & img);
	void Draw(cv::Mat & img, VE::Transform2D & t);
};