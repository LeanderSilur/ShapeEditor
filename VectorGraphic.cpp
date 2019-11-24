#include "VectorGraphic.h"

#include <iostream>
#include <sstream>
#include <fstream>

bool VectorGraphic::closestPointInRange(const VE::Point & center, std::vector<PolylinePointer> Polylines,
	VE::Point& result, float maxDist2)
{
	bool found = false;

	for (auto&polyline: Polylines)
	{
		int index = polyline->PointIndex(center, maxDist2);
		if (index >= 0)
		{
			VE::Point otherPt = polyline->getPoint(index);
			VE::Point direction = otherPt - center;
			float dist2 = direction.x * direction.x + direction.y * direction.y;

			if (dist2 < maxDist2) {
				found = true;
				result = otherPt;
				maxDist2 = dist2;
			}
		}
	}

	return found;
}
using namespace VE;

std::vector<Connection> VectorGraphic::GetConnections(const VE::Point& pt, const std::vector<PolylinePointer>& polylines)
{
	std::vector<Connection> connections;
	for (auto&polyline:polylines)
	{
		Connection connection;
		connection.polyline = polyline;

		if ((*polyline).Front() == pt) {
			connection.at = Connection::start;
			connections.push_back(connection);
		}
		if ((*polyline).Back() == pt) {
			connection.at = Connection::end;
			connections.push_back(connection);
		}
	}
	return connections;
}

inline float Mag(const VE::Point& pt)
{
	return std::sqrt(pt.x * pt.x + pt.y * pt.y);
}

inline float Angle(const VE::Point& pt, const VE::Point& a, const VE::Point& b)
{
	VE::Point a_ = a - pt;
	VE::Point b_ = b - pt;

	float dot_len = (a_.x * b_.x + a_.y * b_.y) / (Mag(a_) * Mag(b_));
	return std::acos(dot_len);
}

inline float VectorGraphic::GetConnectionAngle(const VE::Connection& conA, const VE::Connection& conB)
{
	VE::Point pt, a, b;
	if (conA.at == VE::Connection::start) {
		pt = conA.polyline->Back();
		a = conA.polyline->Back1();
	}
	else {
		pt = conA.polyline->Front();
		a = conA.polyline->Front1();
	}

	if (conB.at == VE::Connection::start)
		b = conB.polyline->Front1();
	else
		b = conB.polyline->Back1();

	return Angle(pt, a, b);
}




const char * quotationMark = "\"'";
// Parse points from svg line.
void getPoints(std::vector<VE::Point> & points, std::string line)
{
	if (line.length() == 0) return;

	std::string openingTag = "<polyline points=";
	size_t start = line.find(openingTag);
	if (start == std::string::npos) return;

	// get the separating character
	start = line.find_first_not_of(" \t", openingTag.size() + start);
	if (start == std::string::npos) return;

	// check that the separator is a quotation mark
	if (line.find_first_of(quotationMark, start) != start) return;

	// get next quotation mark
	size_t end = line.find(line.substr(start, 1), start + 1);
	if (end == start || end == std::string::npos) return;

	//verify number sequence
	size_t unidentified = line.find_first_not_of("0123456789,. \t", start + 1);
	if (unidentified != end) {
		std::cout << "Invalid tag."<< start << " " <<unidentified << "\n";
		return;
	}
	size_t pos = start + 1;
	size_t commaPos,
		   spacePos;


	while (pos < end) {
		commaPos = line.find(',', pos);
		if (commaPos == std::string::npos) return;

		spacePos = line.find(' ', commaPos + 1);
		if (spacePos >= end)
			spacePos = end;

		VE::Point pt;
		pt.x = std::atof(line.substr(pos, commaPos - pos).c_str());
		pt.y = std::atof(line.substr(commaPos + 1, spacePos - commaPos - 1).c_str());
		points.push_back(pt);
		pos = spacePos;
	}

}

void VectorGraphic::LoadPolylines(std::string svgPath)
{
	std::string line;
	std::ifstream myfile(svgPath);
	if (myfile.is_open())
	{
		int i = 0;
		while (getline(myfile, line))
		{

			std::vector<VE::Point> pts;
			getPoints(pts, line);

			if (pts.size() >= 2 ) {
				// TODO remove limit
				if (i < 50){
					std::shared_ptr< VE::Polyline> ptr = std::make_shared<VE::Polyline>(pts);
					Polylines.push_back(ptr);
				}
				i++;
			}

		}
		myfile.close();
	}
	std::cout << "Snapping...";
	SnapEndpoints();
	std::cout << " done\n";
	std::cout << "Overlaps...";
	RemoveOverlaps();
	std::cout << " done\n";
	std::cout << "Merging...";
	MergeConnected();
	std::cout << " done\n";

	
	for (PolylinePointer& p : Polylines) p->SimplifyNth(1.0);
	for (PolylinePointer& p : Polylines) p->Smooth(10, 0.5);
	for (PolylinePointer& p : Polylines) p->Cleanup();
	ComputeConnections();


	//RemoveMaxLength();
}

// Snap endpoints of curves
void VectorGraphic::SnapEndpoints()
{
	for (size_t i = 0; i < Polylines.size(); i++)
	{
		auto& polyline = Polylines[i];
		auto& points = polyline->getPoints();
		// Construct a new Vector with references to all the other polylines
		decltype(Polylines) otherLines(Polylines);
		otherLines.erase(otherLines.begin() + i);

		bool lineChanged = false;
		VE::Point &start = points.front();
		VE::Point &end = points.back();


		VE::Point closest;

		if (closestPointInRange(start, otherLines, closest, SNAPPING_DISTANCE2)) {
			lineChanged = true;
			start = closest;
		}
		if (closestPointInRange(end, otherLines, closest, SNAPPING_DISTANCE2)) {
			lineChanged = true;
			end = closest;
		}
		if (lineChanged) {
			polyline->Cleanup();
		}
	}
}


// trace in direction to determine overlap
// starting at lineA[a] == lineB[b]
void TraceOverlap(const std::vector<VE::Point>& lineA, const std::vector<VE::Point>& lineB, int& a, int b) {
	int traceDir;
	int exitB;

	// trace B forwards (else backwards)
	if (lineB.size() != b + 1 && lineA[a + 1] == lineB[b + 1]) {
		traceDir = 1;
		exitB = lineB.size() - 1;
	}
	else if (-1 != b - 1 && lineA[a + 1] == lineB[b - 1]) {
		traceDir = -1;
		exitB = 0;
	}
	else {
		// only initial point is the same
		return;
	}

	do {
		a++;
		b += traceDir;
	} while (
		a != lineA.size() &&
		b != exitB &&
		lineA[a] == lineB[b]
		);

	return;
}

// Remove floats (Curves Segments which overlap)
// and split curves into separate segments at intersections
void VectorGraphic::RemoveOverlaps()
{
	size_t numberOfPolylines = Polylines.size();
	for (size_t i = 0; i < numberOfPolylines; i++)
	{
		// Take one out.
		PolylinePointer polyline = Polylines[0];
		Polylines.erase(Polylines.begin());

		std::vector<VE::Point>& points = polyline->getPoints();
		decltype(Polylines) newPolylines;

		for (size_t j = 0; j < points.size() - 1; j++)
		{
			// Compare to all the other polylines
			for (auto otherLine = Polylines.begin(); otherLine != Polylines.end(); otherLine++)
			{
				int indexB = (*otherLine)->PointIndex(points[j]);

				// indexB is >= 0 if there a same point from an otherLine
				if (indexB >= 0) {
					// Chop the first segments, if we aren't at the first point anymore,
					// and save it in the new polyline list.
					if (j > 0) {
						//std::cout << "Chop at " << j << "/" << points.size() << "\n";
						std::vector<VE::Point> choppedPoints(points.begin(), points.begin() + j + 1);
						newPolylines.push_back(std::make_shared<VE::Polyline>(choppedPoints));
					}

					int end = j;
					// Track to the "end" of the overlap and remove the points before the end.
					TraceOverlap(points, (*otherLine)->getPoints(), end, indexB);
					if (end > 1) {
						//std::cout << "Removing overlap: " << j << " - " << end << "\n";

						// Removing from the middle.
						points.erase(points.begin(), points.begin() + end);
						j = -1;
						// break to the start of looping through all points
						// (which are left) and looping through all other lines.
						break;
					}
				}
			}

			if (points.size() == 0)
				break;
		}

		// Optimization, if the line hasn't been split yet, just append the original line.
		if (newPolylines.size() == 0) {
			Polylines.push_back(polyline);
		}
		else {
			if (points.size() > 1) {
				newPolylines.push_back(std::make_shared<Polyline>(points));
			}
			Polylines.insert(Polylines.end(), newPolylines.begin(), newPolylines.end());
		}
		std::cout << "|";
	}
}

void VectorGraphic::MergeConnected()
{
	int totalLength = Polylines.size();
	for (int i = Polylines.size() - 1; i >= 0; )
	{
		PolylinePointer mainPolyline = Polylines[0];
		std::cout << mainPolyline->debug << "\n";
		std::vector<Connection> connections;

		// create first item
		std::vector<Connection> nextConnections;
		nextConnections.push_back(Connection());
		nextConnections[0].at = Connection::start;
		nextConnections[0].polyline = mainPolyline;
		Point LoopConnect = mainPolyline->Front();

		auto otherLines = Polylines;
		while (true) {
			// Add the con(nection) to connections.
			Connection con = nextConnections[0];
			connections.push_back(con);
			PolylinePointer polyline = con.polyline;

			// Remove the item from the otherLines
			otherLines.erase(std::find(otherLines.begin(), otherLines.end(), con.polyline));

			// Determine the endPoint for the next search.
			Point searchPoint = con.EndPoint();
			if (searchPoint == LoopConnect) break;

			// Do the search.
			nextConnections = GetConnections(searchPoint, otherLines);
			if (nextConnections.size() != 1) break;
			// Compare angles.
			Connection& next = nextConnections[0];
			if (GetConnectionAngle(con, next) < MIN_MERGE_ANGLE) {
				break;
			}
		}

		// Remove the first (main) element and do the search backwards.
		nextConnections = std::vector<Connection>();
		nextConnections.push_back(Connection());
		nextConnections[0].at = Connection::start;
		nextConnections[0].polyline = connections[0].polyline;
		LoopConnect = connections.back().EndPoint();

		connections.erase(connections.begin());

		otherLines = Polylines;
		while (true) {
			Connection con = nextConnections[0];
			connections.insert(connections.begin(), con);

			PolylinePointer polyline = con.polyline;

			otherLines.erase(std::find(otherLines.begin(), otherLines.end(), polyline));

			// Inverse for backwards search.
			Point searchPoint = con.StartPoint();
			if (searchPoint == LoopConnect) break;

			// Do the search.
			nextConnections = GetConnections(searchPoint, otherLines);
			if (nextConnections.size() != 1) break;

			nextConnections[0].Invert();
			// Compare angles.
			Connection& next = nextConnections[0];
			if (GetConnectionAngle(next, con) < MIN_MERGE_ANGLE) {
				break;
			}
		}

		// Finally, check if we have Connections and if yes
		// remove them from the main list.
		if (connections.size() > 1) {
			// Contruct a new Point Sequence
			std::vector<Point> points;
			std::cout << mainPolyline->debug << "\n";
			for (Connection& con: connections)
			{
				std::cout << "           " << con.polyline->debug << "\n";
				std::vector<Point>& newPoints = con.polyline->getPoints();
				if (con.at == Connection::start) {
					// this will create doubles.
					points.insert(points.end(), newPoints.begin(), newPoints.end());
				}
				else {
					points.insert(points.end(), newPoints.rbegin(), newPoints.rend());
				}
				auto & it = std::find(Polylines.begin(), Polylines.end(), con.polyline);
				if (it == Polylines.end()) std::cout << "not found\n";

				Polylines.erase(it);
			}
			PolylinePointer newPolyline = std::make_shared<Polyline>();
			newPolyline->setPoints(points);
			Polylines.push_back(newPolyline);
		}
		else {
			std::cout << " - (rotating)\n";
			std::rotate(Polylines.begin(), Polylines.begin() + 1, Polylines.end());
		}
		// Decrease i by as many items as we remove (connections.size())).
		i -= connections.size();
	}
}

void VectorGraphic::ComputeConnections()
{
	for (PolylinePointer& pl:Polylines)
	{
		auto otherLines = Polylines;
		otherLines.erase(std::find(otherLines.begin(), otherLines.end(), pl));

		// compute front
		pl->ConnectFront = GetConnections(pl->Front(), otherLines);
		pl->ConnectBack = GetConnections(pl->Back(), otherLines);
		pl->UpdateStatus();
	}
}

void VectorGraphic::ClosestElement(cv::Mat & img, VE::Transform2D & t, float & distance, const VE::Point & pt,
	VE::Point & closest, std::shared_ptr<VE::VectorElement>& element)
{
	cv::Rect2f bounds(-t.x / t.scale, -t.y / t.scale, img.cols / t.scale, img.rows / t.scale);
	int id = 0,
		closest_id = 0;
	for (auto el = Polylines.begin(); el != Polylines.end(); el++) {
		VE::VectorElement * ve = el->get();
		if (ve->AnyPointInRect(bounds)) {
			float previous = distance;
			el->get()->Closest2(pt, distance, closest);
			if (previous != distance) {
				element = *el;
				closest_id = id;
			}
		}
		id++;
	}
	//std::cout << "closest " << closest_id << ", " << closest << "\n";
}

void VectorGraphic::Draw(cv::Mat & img)
{
	for (auto el = Polylines.begin(); el != Polylines.end(); el++){
		VE::VectorElement * ve = el->get();
		el->get()->Draw(img);
	}
}

void VectorGraphic::Draw(cv::Mat & img, VE::Transform2D& t)
{
	cv::Rect2f bounds(-t.x / t.scale, -t.y / t.scale, img.cols / t.scale, img.rows / t.scale);

	for (auto el = Polylines.begin(); el != Polylines.end(); el++) {
		VE::VectorElement * ve = el->get();
		if (ve->AnyPointInRect(bounds)) {
			el->get()->Draw(img, t);
		}
	}
}
