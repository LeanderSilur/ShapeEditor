#include "Polyshape.h"

#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "Polyline.h"


namespace VE {

	void Polyshape::Cleanup()
	{
		points.clear();
		for (Connection& con : connections)
		{
			con.AppendTo(points, 1);
		}

		bounds = Bounds();
		calcBounds(points);
	}

	Polyshape::Polyshape()
	{
	}

	std::vector<Point>& Polyshape::getPoints()
	{
		return this->points;
	}

	void Polyshape::setConnections(std::vector<Connection>& connections)
	{
		// Validating connections.
		if (connections.size() == 0 ||
			(connections.size() == 1 && connections[0].EndPoint() != connections[0].StartPoint()))
			throw std::invalid_argument("Not gud.");

		for (auto con = connections.begin() + 1; con != connections.end(); con++)
		{
			if ((con - 1)->EndPoint() != con->StartPoint()) {
				throw std::invalid_argument("Not gud.");
			}
		}

		// all good
		this->connections = connections;
	}

	std::vector<Connection>& Polyshape::getConnections()
	{
		return connections;
	}


	void Polyshape::Draw(cv::Mat& img, Transform& t, const cv::Scalar& color)
	{
		// This is copy-pasta from Polyline, fix this. [TODO]

		// determine the minimum distance between two points
		float minDist = MIN_DRAWING_DISTANCE;
		t.applyInv(minDist);
		float minDist2 = minDist * minDist;
		
		// Create a copy, then simplify it according to the Transforms mindist2.
		std::vector <VE::Point> drawPoints;
		for (auto&con: connections)
		{
			auto& pts = con.polyline->getSimplified(minDist2);
			if (con.at == Connection::Location::start) {
				drawPoints.insert(drawPoints.end(), pts.begin() + 1, pts.end());
			}
			else {
				drawPoints.insert(drawPoints.end(), pts.rbegin() + 1, pts.rend());
			}
		}

		for (auto& pt : drawPoints)
			t.apply(pt);

		std::vector<cv::Point2i> tmp;
		cv::Mat(drawPoints).copyTo(tmp);

		// This will fail if tmp is empty.
		const cv::Point2i* elementPoints[1] = { &tmp[0] };
		int numberOfPoints = (int)tmp.size();


		cv::fillPoly(img, elementPoints, &numberOfPoints, 1, color, cv::LINE_AA);
		//drawBoundingBox(img, t);
	}

	bool Polyshape::AnyPointInRect(Bounds& other)
	{
		return bounds.Overlap(other);
	}

}