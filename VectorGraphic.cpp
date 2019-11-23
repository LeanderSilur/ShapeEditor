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

std::vector<Connection> VectorGraphic::GetConnections(const VE::Point& pt, const std::vector<PolylinePointer>& polylines)
{
	std::vector<Connection> connections;
	for (auto&polyline:polylines)
	{
		bool connected = false;
		Connection connection;
		connection.at = Connection::none;

		if ((*polyline).Front() == pt)
			connection.at = Connection::start;
		else if ((*polyline).Back() == pt)
			connection.at = Connection::end;

		if (connection.at != Connection::none) {
			connection.polyline = polyline;
			connections.push_back(connection);
		}
	}
	return connections;
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
				if (true || i < 50){
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
	//MergeConnected();
	std::cout << " done\n";
	//RemoveIntersections();

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
				newPolylines.push_back(std::make_shared<VE::Polyline>(points));
			}
			Polylines.insert(Polylines.end(), newPolylines.begin(), newPolylines.end());
		}
		std::cout << "|";
	}
}

void VectorGraphic::MergeConnected()
{
	decltype(Polylines) newPolylines;
	std::vector<std::vector<VE::Point>> point_list;
	
	while ( Polylines.size() > 0)
	{
		PolylinePointer mainPolyline = Polylines[0];
		std::vector<Connection> connections;

		// create first item
		std::vector<Connection> nextConnections;
		nextConnections.push_back(Connection());
		nextConnections[0].at = Connection::start;
		nextConnections[0].polyline = mainPolyline;


		auto otherLines = Polylines;
		while (nextConnections.size() == 1) {
			// Add the con(ection) to connections.
			Connection& con = nextConnections[0];
			connections.push_back(con);
			PolylinePointer polyline = con.polyline;
			// Determine the endPoint for the next search.
			VE::Point& searchPoint = polyline->Back();
			if (con.at == Connection::end)
				searchPoint = polyline->Front();

			// Remove the item from the otherLines
			otherLines.erase(std::find(otherLines.begin(), otherLines.end(), polyline));

			// Do the search.
			nextConnections = GetConnections(searchPoint, otherLines);
		}

		// Remove the first (main) element and do the search backwards.
		nextConnections = std::vector<Connection>();
		nextConnections.push_back(Connection());
		nextConnections[0].at = Connection::end;
		nextConnections[0].polyline = connections[0].polyline;

		connections.erase(connections.begin());
		otherLines = Polylines;
		while (nextConnections.size() == 1) {
			Connection& con = nextConnections[0];
			con.Invert();
			connections.insert(connections.begin(), con);

			PolylinePointer polyline = con.polyline;
			// Inverse for backwards search.
			VE::Point& searchPoint = polyline->Back();
			if (con.at == Connection::start)
				searchPoint = polyline->Front();

			otherLines.erase(std::find(otherLines.begin(), otherLines.end(), polyline));

			// Do the search.
			nextConnections = GetConnections(searchPoint, otherLines);
		}

		// Finally, check if we have Connections and if yes
		// remove them from the main list.
		if (connections.size() > 1) {
			// Contruct a new Point Sequence
			std::vector<VE::Point> points;
			for (Connection& con: connections)
			{
				std::vector<VE::Point>& newPoints = con.polyline->getPoints();
				if (con.at == Connection::start) {
					// this will create doubles.
					points.insert(points.end(), newPoints.begin(), newPoints.end());
				}
				else {
					points.insert(points.end(), newPoints.rbegin(), newPoints.rend());
				}
				Polylines.erase(std::find(Polylines.begin(), Polylines.end(), con.polyline));
			}
			PolylinePointer newPolyline = std::make_shared<VE::Polyline>();
			newPolyline->setPoints(points);
			newPolylines.push_back(newPolyline);
		}
		else {
			newPolylines.push_back(Polylines[0]);
			Polylines.erase(Polylines.begin());
		}
	}
	Polylines = newPolylines;
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
