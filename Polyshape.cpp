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


	void Polyshape::Draw(cv::Mat& img, Transform& t, bool highlight)
	{
		if (points.size() < 2) {
			std::cout << "Polyshape not calculated yet. Do a cleanup first!\n";
			throw std::invalid_argument("Polyshape not calculated yet. Do a cleanup first!");
		}

		cv::Scalar color(200, 200, 170);


		// determine the minimum distance between two points
		float minDist = MIN_DRAWING_DISTANCE;
		t.applyInv(minDist);
		float minDist2 = minDist * minDist;


		// Create a copy, then simplify it according to the Transforms mindist2.
		auto drawPoints = points;
		SimplifyNth(drawPoints, minDist2);

		for (auto& pt : drawPoints)
			t.apply(pt);

		std::vector<cv::Point2i> tmp;
		cv::Mat(drawPoints).copyTo(tmp);

		// This will fail if tmp is empty.
		const cv::Point2i* elementPoints[1] = { &tmp[0] };
		int numberOfPoints = (int)tmp.size();

		//Mat a = (Mat_<int>(4, 2) << 0, 1, 10, 11, 20, 21, 30, 31);


		cv::fillPoly(img, elementPoints, &numberOfPoints, 1, color, cv::LINE_AA);

		drawBoundingBox(img, t);

	}

	bool Polyshape::AnyPointInRect(Bounds& other)
	{
		return bounds.Overlap(other);
	}

	Bounds& Polyshape::getBounds()
	{
		return Bounds();
	}

}