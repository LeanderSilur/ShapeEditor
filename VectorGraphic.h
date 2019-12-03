#pragma once

#include <vector>
#include "VectorElement.h"

class VectorGraphic {
private:


	// returns the closest point on the polyline
	bool closestPointInRange(const VE::Point & center,
		std::vector<VE::PolylinePtr> Polylines,
		VE::Point& result, float maxDist2);
	
	std::vector<VE::Connection> GetConnections(const VE::Point& pt, const std::vector<VE::PolylinePtr>& polylines);
	inline float GetConnectionAngle(const VE::Connection& conA, const VE::Connection& conB);

	void DeleteConnections(VE::PolylinePtr ptr);
public:
	// Used in SnapEndpoints(). The distance2 of an endpoint to a
	// potential intersection point.
	float SNAPPING_DISTANCE2 = 4;
	// Used in MergeConnected(). If two linesegments share an enpoint
	// they can be connected if this angle isn't exceeded.
	// PI * 0.5 ^= 90°
	float MIN_MERGE_ANGLE = CV_PI * 0.75;

	// Used in polyline smoothing.
	int SMOOTHING_ITERATIONS = 10;
	float SMOOTHING_LAMBDA = 0.5f;

	// Used in polyline simplification. Points closer together
	// than this value will be merged together in Polyline::Simplify()
	float SIMPLIFY_MAX_DIST = 1.2f;


	VectorGraphic() {};
	

	std::vector<VE::PolylinePtr> Polylines;
	std::vector<VE::PolyshapePtr> Polyshapes;

	void LoadPolylines(std::string path);

	void SnapEndpoints();
	void RemoveOverlaps();
	void MergeConnected();
	void ComputeConnectionStatus();
	void RemoveUnusedConnections();
	void CalcShapes();


	void Split(VE::PolylinePtr pl, VE::Point pt);
	void Connect(VE::Connection& a, VE::Connection& b);
	void Delete(VE::PolylinePtr line);

	// bloated complicated method, used for the imageviewer
	// returns true if a point closer than distance2 was found
	void ClosestPolyline(cv::Mat& img, VE::Transform& t, float& distance2, const VE::Point& pt,
		VE::Point& closest, VE::PolylinePtr& element);
	void ClosestPolyline(VE::Bounds& b, float& distance2, const VE::Point& pt,
		VE::Point& closest, VE::PolylinePtr& element);

	void Draw(cv::Mat & img, VE::Transform& t);
};