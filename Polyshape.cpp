#include "Polyshape.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "Polyline.h"
#include "Connection.h"
#include "ColorArea.h"


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

	VE::PolyshapePtr Polyshape::FromData(std::vector<PolylinePtr>& lines, PolyshapeData& shapeData)
	{
		PolyshapePtr ptr = std::make_shared<Polyshape>();

		std::vector<Connection> connections;
		for (auto& p : shapeData.data) {
			if (p.first >= lines.size() || p.second < 0 || p.second > 1) {
				return nullptr;
			}

			Connection con(lines[p.first], (VE::Connection::Location)p.second);
			connections.push_back(con);
		}

		try {
			ptr->setConnections(connections);
		}
		catch (const std::logic_error & e) {
			return nullptr;
		}
		ptr->setColor(shapeData.color);
		ptr->Cleanup();
		return ptr;
	}

	std::vector<Point>& Polyshape::getPoints()
	{
		return this->points;
	}

	VE::ColorAreaPtr Polyshape::getColor()
	{
		return color;
	}

	void Polyshape::setColor(VE::ColorAreaPtr& col)
	{
		color = col;
	}

	void Polyshape::setConnections(std::vector<Connection>& connections)
	{
		// Validating connections.
		if (connections.size() == 0 ||
			(connections.size() == 1 && connections[0].EndPoint() != connections[0].StartPoint())) {
			valid = false;
			throw std::invalid_argument("Not gud.");
		}

		for (auto con = connections.begin() + 1; con != connections.end(); con++)
		{
			if ((con - 1)->EndPoint() != con->StartPoint()) {
				throw std::logic_error("Last point is not the same as first");
			}
		}

		// All good.
		this->connections = connections;
		CalculateDirection();
		valid = true;
	}

	const std::vector<Connection>& Polyshape::getConnections()
	{
		return connections;
	}


	void Polyshape::Draw(cv::Mat& img, Transform& t)
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

		cv::fillPoly(img, elementPoints, &numberOfPoints, 1, color->Color, cv::LINE_AA);
		//drawBoundingBox(img, t);
	}

	bool Polyshape::AnyPointInRect(Bounds& other)
	{
		return bounds.Overlap(other);
	}

	void Polyshape::CalculateDirection()
	{
		float angle = 0;
		for (auto&con:connections)
			angle += con.AngleArea();

		counterClockwise = angle > 0;
	}

	bool Polyshape::CounterClockwise()
	{
		return counterClockwise;
	}

}