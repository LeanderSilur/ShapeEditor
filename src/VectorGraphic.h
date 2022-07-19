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
	static const float MERGE_DISTANCE;
	static const float MERGE_DISTANCE2;

	VectorGraphic() { };

	std::vector<VE::PolylinePtr> Polylines;
	std::vector<VE::PolyshapePtr> Polyshapes;

	VE::PolylinePtr AddPolyline(const std::vector<VE::Point>& pts);
	void AddPolylines(const std::vector<std::vector<VE::Point>>& ptsList);
	void Load(std::string path);
	void Save(std::string path, std::string image_path, cv::Size2i shape);
	void SavePolyshapes(std::string path, std::string image_path, cv::Size2i shape);

	void SnapEndpoints(const float& snappingDistance2);
private: void GetNoneOverlappingLines(std::vector<VE::Point>& points, std::vector<VE::PolylinePtr>& result,
	const std::vector<VE::PolylinePtr> others, const std::vector<VE::Bounds> paddedBounds, const std::vector<float> distances2);
public:
	void RemoveOverlaps();
	void MergeConnected(const float& minMergeAngle);
	void ComputeConnectionStatus();
	void ComputeConnectionStatus(VE::PolylinePtr pl);
	void RemoveUnusedConnections();

	// Lines
	void Split(VE::PolylinePtr pl, const int& idx);
	void Split(VE::PolylinePtr pl, const int& idx, const float& t);
	void Split(VE::PolylinePtr pl, const std::vector<int> indices);
	//void Connect(VE::Connection& a, VE::Connection& b);
	void Delete(VE::PolylinePtr line);

	// Shapes
private:
	void TraceSingleShape(std::vector<VE::Connection>& connections, VE::PolyshapePtr& result);
	void RemovePolyshapeWith(VE::PolylinePtr ptr);
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


	struct CPParams
	{
		enum M { Point, Segment, Split };
		CPParams::M method = CPParams::M::Point;

		CPParams(const VE::Point& target, CPParams::M method = CPParams::M::Point);
		float distance2;
		int ptIdx;
		float t;
		VE::Point closestPt;
		VE::PolylinePtr closestPolyline;

		const VE::Point& target;
		float snapEndpoints2 = 0.f;
		const VE::Bounds* bounds = nullptr;
	};
	
	// returns the points index if a point closer than distance2 was found
	// If there was no point, the index -1 is returned.
	void ClosestPolyline(CPParams& params, std::vector<VE::PolylinePtr> lines);
	void ClosestPolyline(CPParams& params);

private: 
	bool ClosestConnectionLeft(const VE::Point& target, std::vector<VE::PolylinePtr>& lines, VE::Connection& result);
public:
	void ClosestPolyshape(const VE::Point& pt, VE::PolyshapePtr& element);

	void ClosestEndPoint(cv::Mat& img, VE::Transform& t, float& maxDist2, const VE::Point& pt, VE::Point& closest);
	void ClosestEndPoint(VE::Bounds& b, float& maxDist2, const VE::Point& pt, VE::Point& closest);

	void Draw(cv::Mat & img, VE::Transform& t);
	VE::Bounds getBounds();
};