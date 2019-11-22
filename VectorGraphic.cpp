#include "VectorGraphic.h"

#include <iostream>
#include <sstream>
#include <fstream>

std::vector<ConnectionStart> VectorGraphic::GetConnected(std::vector<std::shared_ptr<VE::Polyline>>::iterator el, VE::Point & pt)
{
	std::vector<ConnectionStart> connectionStarts;
	for (el; el != Polylines.end(); el++) {
		bool connected = false;
		ConnectionStart connection;

		if ((*el)->FrontPoint() == pt) {
			connected = true;
			connection.fromBackPoint = false;
		}
		if ((*el)->BackPoint() == pt) {
			connected = true;
			connection.fromBackPoint = true;
		}

		if (connected) {
			connection.polylineIt = el;
			connectionStarts.push_back(connection);
		}
	}
	return connectionStarts;
}

bool VectorGraphic::closestPointInRange(const VE::Point & center, std::vector<PolylinePointer> Polylines, VE::Point& result, double maxDist2)
{
	bool found = false;

	for (auto&polyline: Polylines)
	{
		int index = polyline->PointIndex(center, maxDist2);
		if (index >= 0)
		{
			VE::Point otherPt = polyline->getPoint(index);
			VE::Point direction = otherPt - center;
			double dist2 = direction.x * direction.x + direction.y * direction.y;

			if (dist2 < maxDist2) {
				found = true;
				result = direction;
				maxDist2 = dist2;
			}
		}
	}

	return found;
}


VectorGraphic::VectorGraphic()
{
}

VectorGraphic::VectorGraphic(const VectorGraphic& other)
{
	Polylines = other.Polylines;
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
			else {
				std::cout << "Some problem occured.\n";
			}

		}
		myfile.close();
	}
	//RemoveOverlaps();
	//MergeConnected();
	//RemoveIntersections();

	//RemoveMaxLength();
}

// Snap endpoints of curves
void VectorGraphic::SnapEndpoints()
{/*
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
			std::cout << "Snapped endpoint(s) of line " << i << "\n";
			polyline->Cleanup();
		}
	}*/
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

// Remove Doubles (Curves Segments which overlap)
// and split curves into separate segments at intersections
void VectorGraphic::RemoveOverlaps()
{
	decltype(Polylines) newPolylines;

	for (size_t i = 0; i < Polylines.size(); i++)
	{
		// Take one out.
		auto & polyline = Polylines[i];
		std::vector<VE::Point>& points = polyline->getPoints();
		
		for (size_t j = 0; j < points.size() - 1; j++)
		{
			// Compare to all the other polylines
			for (auto otherLine = Polylines.begin() + i + 1; otherLine != Polylines.end(); otherLine++)
			{
				int indexB = (*otherLine)->PointIndex(points[j]);
				// indexB is >= 0 if there a same point from an otherLine
				if (indexB >= 0) {
					// Split the vector first, to chop the untouched first part.
					// This will happen independently of wether the next points are connected.
					if (j > 0) {
						std::vector<VE::Point> choppedPoints(points.begin(), points.begin() + j + 1);
						newPolylines.push_back(std::make_shared<VE::Polyline>(choppedPoints));
					}

					int end = j;
					TraceOverlap(points, (*otherLine)->getPoints(), end, indexB);
					// Check that the segment is longer than 1 point.
					if (end - j > 1) {
						std::cout << "Removing overlap: " << j << " - " << end << "\n";

						// Removing from the middle.
						points.erase(points.begin(), points.begin() + end);
						j = -1;
						// break to the start of looping through all points
						// (which are left) and looping through all other lines.
						break;
					}
					else {
						std::cout << "Splitting line: " << j << "\n";

					}
				}
			}

		}
		if (points.size() > 1)
			newPolylines.push_back(std::make_shared<VE::Polyline>(points));
	}
	Polylines = newPolylines;
}

void VectorGraphic::RemoveIntersections()
{
	std::cout << ">>> Removing Intersections\nSize:   " << Polylines.size() << "\n";

	for (int i = 0; i < Polylines.size() - 1; i++) {
		VE::Polyline & first = *Polylines[i];
		std::cout << Polylines.size() << ": " << i << "                              \r";

		for (int j = i + 1; j < Polylines.size(); j++)
		{
			VE::Polyline & second = *Polylines[j];
			std::shared_ptr<VE::Polyline> newEl = first.splitIntersecting(second);

			//	std::cout << Polylines.size() << ": " << i << " x " << j << " > " << (newEl != nullptr) << "                              \n";

			if (newEl != nullptr) {
				Polylines.push_back(newEl);
			}
		}
	}
	std::cout << "Result   " << Polylines.size() << "\n";
}

void VectorGraphic::RemoveMaxLength(int length)
{
	std::cout << ">>> Removing < Maxlen\nSize:   " << Polylines.size() << "\n";
	// TODO connection check
	auto el = Polylines.begin();
	while (el != Polylines.end()) {
		if ((*el)->Length() <= length) {
			el = Polylines.erase(el);
		}
		else
		{
			el++;
		}
	}
	std::cout << "Done\n";
}


void VectorGraphic::MergeConnected()
{
	std::cout << "Starting with " << Polylines.size() << "\n";
	for (auto el1 = Polylines.begin(); el1 != Polylines.end() - 1; ) {

		VE::Point front = (*el1)->FrontPoint();
		VE::Point back = (*el1)->BackPoint();


		std::vector<ConnectionStart> csFront = GetConnected(el1 + 1, front);
		if (csFront.size() == 1) {
			ConnectionStart connection = csFront[0];
			(*el1)->PrependMove(**connection.polylineIt, connection.fromBackPoint);
			Polylines.erase(connection.polylineIt);
		}

		std::vector<ConnectionStart> csBack = GetConnected(el1 + 1, back);
		if (csBack.size() == 1) {
			ConnectionStart connection = csBack[0];
			(*el1)->AppendMove(**connection.polylineIt, connection.fromBackPoint);
			Polylines.erase(connection.polylineIt);
		}
		// Only advance if nothing changed.
		if (csFront.size() != 1 && csBack.size() != 1)
			el1++;
	}

	std::cout << "Ending with " << Polylines.size() << "\n";
}

void VectorGraphic::ClosestElement(cv::Mat & img, VE::Transform2D & t, double & distance, const VE::Point & pt,
	VE::Point & closest, std::shared_ptr<VE::VectorElement>& element)
{
	cv::Rect2d bounds(-t.x / t.scale, -t.y / t.scale, img.cols / t.scale, img.rows / t.scale);
	int id = 0,
		closest_id = 0;
	for (auto el = Polylines.begin(); el != Polylines.end(); el++) {
		VE::VectorElement * ve = el->get();
		if (ve->AnyPointInRect(bounds)) {
			double previous = distance;
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
	cv::Rect2d bounds(-t.x / t.scale, -t.y / t.scale, img.cols / t.scale, img.rows / t.scale);

	for (auto el = Polylines.begin(); el != Polylines.end(); el++) {
		VE::VectorElement * ve = el->get();
		if (ve->AnyPointInRect(bounds)) {
			el->get()->Draw(img, t);
		}
	}
}
