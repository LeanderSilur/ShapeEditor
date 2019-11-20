#pragma once

#include <vector>
#include <opencv2/core.hpp>

#include "VectorElements.h"


typedef struct ConnectionStart {
public:
	std::vector<std::shared_ptr<VE::Polyline>>::iterator polylineIt;
	bool fromBackPoint;
};
class VectorGraphic {

private:
	std::vector<ConnectionStart> GetConnected(std::vector<std::shared_ptr<VE::Polyline>>::iterator startElement, VE::Point &pt);
public:

	VectorGraphic();
	

	std::vector<std::shared_ptr<VE::Polyline>> Polylines;

	void LoadPolylines(std::string path);

	void RemoveIntersections();
	void RemoveMaxLength(int length = 12);
	void MergeConnected();

	void ClosestPoint(cv::Mat& img, VE::Transform2D& t, double& distance, VE::Point& pt,
		VE::Point& closest, std::shared_ptr<VE::VectorElement>& element);

	void Draw(cv::Mat & img);
	void Draw(cv::Mat & img, VE::Transform2D & t);
};