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

	VectorGraphic() { };

	std::vector<VE::PolylinePtr> Polylines;
	std::vector<VE::PolyshapePtr> Polyshapes;

	void AddPolyline(std::vector<VE::Point>& pts);
	void Load(std::string path);
	void Save(std::string path, std::string image_path, cv::Size2i shape);
	void SavePolyshapes(std::string path, std::string image_path, cv::Size2i shape);

	void SnapEndpoints(const float& snappingDistance2);
	void RemoveOverlaps();
	void MergeConnected(const float& minMergeAngle);
	void ComputeConnectionStatus();
	void RemoveUnusedConnections();

	// Lines
	void Split(VE::PolylinePtr pl, VE::Point pt);
	//void Connect(VE::Connection& a, VE::Connection& b);
	void Delete(VE::PolylinePtr line);

	// Shapes
private:
	void TraceSingleShape(std::vector<VE::Connection>& connections, VE::PolyshapePtr& result);
public:
	void CalcShapes();
	void ColorShapesRandom();
	void SortShapes();
private:
	void SortShapes(std::vector<VE::PolyshapePtr> & shapes);

public:
	void ClearShapes();
	VE::PolyshapePtr CreateShape(const VE::Point & target);
	VE::PolyshapePtr ColorShape(const VE::Point& pt, VE::ColorAreaPtr& color);
	void PickColor(const VE::Point& pt, VE::ColorAreaPtr& color);
	bool DeleteShape(const VE::Point& pt);

	void MakeColorsUnique();



	// bloated complicated method, used for the imageviewer
	// returns the points index if a point closer than distance2 was found
	// If there was no point, the index -1 is returned.
	int ClosestPolyline(cv::Mat& img, VE::Transform& t, float& distance2, const VE::Point& pt,
		VE::Point& closest, VE::PolylinePtr& element);
	
	int ClosestPolyline(VE::Bounds& b, float& distance2, const VE::Point& pt,
		VE::Point& closest, VE::PolylinePtr& element);

private: 
	bool ClosestConnectionLeft(const VE::Point& target, std::vector<VE::PolylinePtr>& lines, VE::Connection& result);
public:
	void ClosestPolyshape(const VE::Point& pt, VE::PolyshapePtr& element);

	void ClosestEndPoint(cv::Mat& img, VE::Transform& t, float& maxDist2, const VE::Point& pt, VE::Point& closest);
	void ClosestEndPoint(VE::Bounds& b, float& maxDist2, const VE::Point& pt, VE::Point& closest);

	void Draw(cv::Mat & img, VE::Transform& t);
	VE::Bounds getBounds();
};