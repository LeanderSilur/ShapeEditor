#include "VectorGraphic.h"

#include "Polyline.h"
#include "Polyshape.h"
#include "Export.h"

#include <iostream>
#include <sstream>
#include <fstream>

bool VectorGraphic::closestPointInRange(const VE::Point & center, std::vector<VE::PolylinePtr> Polylines,
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

std::vector<Connection> VectorGraphic::GetConnections(const VE::Point& pt, const std::vector<VE::PolylinePtr>& polylines)
{
	std::vector<Connection> connections;
	for (auto&polyline:polylines)
	{
		Connection connection;
		connection.polyline = polyline;

		if (polyline->Front() == polyline->Back()) {
			// Don't use loops.
			continue;
		}
		if ((*polyline).Front() == pt) {
			connection.at = Connection::Location::start;
			connections.push_back(connection);
		}
		if ((*polyline).Back() == pt) {
			connection.at = Connection::Location::end;
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
	if (conA.at == VE::Connection::Location::start) {
		pt = conA.polyline->Back();
		a = conA.polyline->Back1();
	}
	else {
		pt = conA.polyline->Front();
		a = conA.polyline->Front1();
	}

	if (conB.at == VE::Connection::Location::start)
		b = conB.polyline->Front1();
	else
		b = conB.polyline->Back1();

	return Angle(pt, a, b);
}

void VectorGraphic::DeleteConnections(VE::PolylinePtr ptr)
{
	decltype(Polylines) newPolylines;

	for (int i = 0; i < Polylines.size(); i++)
	{
		bool clean = true;
		for (int a = 0; a < Polylines[i]->ConnectFront.size(); a++)
		{
			if (Polylines[i]->ConnectFront[a].polyline == ptr) {
				Polylines[i]->ConnectFront.erase(Polylines[i]->ConnectFront.begin() + a);
				clean = false;
				a--;
			}
		}
		for (int a = 0; a < Polylines[i]->ConnectBack.size(); a++)
		{
			if (Polylines[i]->ConnectBack[a].polyline == ptr) {
				Polylines[i]->ConnectBack.erase(Polylines[i]->ConnectBack.begin() + a);
				clean = false;
				a--;
			}
		}
		if (!clean) {
			Polylines[i]->UpdateStatus();
		}
	}
}




const char* quotationMark = "\"'";
// Parse points from svg line.
void getPoints(std::vector<VE::Point>& points, std::string line)
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
		std::cout << "Invalid tag." << start << " " << unidentified << "\n";
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

// Parse shapes from svg line.
void getSvgShapes(VE::PolyshapeData& shapeData, std::string line)
{
	if (line.length() == 0) return;

	std::string openingTag = "<shape data=";
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
	size_t unidentified = line.find_first_not_of("0123456789, \t", start + 1);
	if (unidentified != end) {
		std::cout << "Invalid tag." << start << " " << unidentified << "\n";
		return;
	}
	size_t pos = start + 1;
	size_t commaPos,
		spacePos;


	while (pos < end - 1) {
		commaPos = line.find(',', pos);
		if (commaPos == std::string::npos) return;

		spacePos = line.find(' ', commaPos + 1);
		if (spacePos >= end)
			spacePos = end;

		std::string si = line.substr(pos, commaPos - pos);
		std::string sj = line.substr(commaPos + 1, spacePos - commaPos - 1);
		int i = std::atoi(si.c_str());
		int j = std::atof(sj.c_str());
		shapeData.data.push_back(std::pair<int, int>(i, j));
		pos = spacePos;
	}
	// Look for ColorInfo
	openingTag = "color=";
	start = line.find(openingTag, end);
	if (start == std::string::npos) return;

	start = line.find_first_not_of(" \t", openingTag.size() + start);
	if (start == std::string::npos) return;

	// check that the separator is a quotation mark
	if (line.find_first_of(quotationMark, start) != start) return;

	// get next quotation mark
	end = line.find(line.substr(start, 1), start + 1);
	if (end == std::string::npos) return;
	
	std::vector<std::string> parts;
	pos = start + 1;
	while (pos < end) {
		commaPos = line.find(',', pos);
		if (commaPos >= end) {
			parts.push_back(line.substr(pos, end - pos));
			break;
		}

		parts.push_back(line.substr(pos, commaPos - pos));
		pos = commaPos + 1;
	}
	if (parts.size() != 4) {
		throw std::exception("");
	}
	shapeData.color = std::make_shared<ColorArea>();
	shapeData.color->Name = parts[0];
	shapeData.color->Color[0] = std::stoi(parts[1]);
	shapeData.color->Color[1] = std::stoi(parts[2]);
	shapeData.color->Color[2] = std::stoi(parts[3]);
}

void VectorGraphic::AddPolyline(std::vector<VE::Point>& pts)
{
	VE::PolylinePtr ptr = std::make_shared<VE::Polyline>(pts);
	Polylines.push_back(ptr);
}

void VectorGraphic::Load(std::string svgPath)
{
	Polylines.clear();
	Polyshapes.clear();

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
				if (true ||i >= 0 && i <= 1){
					AddPolyline(pts);
				}
				i++;
			}
			else {
				VE::PolyshapeData shapeData;
				getSvgShapes(shapeData, line);
				if (!shapeData.data.empty()) {
					VE::PolyshapePtr shape = Polyshape::FromData(Polylines, shapeData);
					if (shape == nullptr) {
						std::cout << "Failed to make shape : " << line << "\n";
					}
					else {
						Polyshapes.push_back(shape);
					}
				}
			}

		}
		myfile.close();
	}
	std::cout << Polyshapes.size() << "\n";
}

void VectorGraphic::Save(std::string path, std::string image_path, cv::Size2i shape)
{
	std::vector<PolyshapeData> shapeDatas;

	for (auto&ps:Polyshapes)
	{
		PolyshapeData shapeData;
		for (auto&con:ps->getConnections())
		{
			auto it = std::find(Polylines.begin(), Polylines.end(), con.polyline);
			if (it == Polylines.end())
			{
				shapeData.data.clear();
				break;
			}
			int index = std::distance(Polylines.begin(), it);
			shapeData.data.push_back(std::pair<int, int>(index, (int)con.at));
		}
		if (!shapeData.data.empty()) {
			shapeData.color = ps->getColor();
			shapeDatas.push_back(shapeData);
		}
	}
	Export::SaveSVG(path, image_path, shape, Polylines, shapeDatas);
}

// Saves the polyshapes, mergeing shapes of the same ColorArea prior.
// Also extract the smaller shapes from larger ones which contain them.
void VectorGraphic::SavePolyshapes(std::string path, std::string image_path, cv::Size2i shape)
{
	// Merge same ColorArea shapes.
	auto saveShapes = Polyshapes;
	decltype(saveShapes) mergedShapes;
	while(!saveShapes.empty())
	{
		std::vector<Connection> cons;
		auto shapeA = std::move(saveShapes.back());
		cons.insert(cons.end(), shapeA->getConnections().begin(), shapeA->getConnections().end());
		saveShapes.pop_back();
		auto& colorArea = shapeA->getColor();

		for (int i = 0; i < saveShapes.size(); i++)
		{
			auto& shapeB = saveShapes[i];
			
			if (colorArea == shapeB->getColor())
			{
				cons.insert(cons.end(), shapeB->getConnections().begin(), shapeB->getConnections().end());
				saveShapes.erase(saveShapes.begin() + i);
				i--;
			}
		}

		// Remove doubles.
		for (int a = 0; a < cons.size(); a++)
		{
			for (int b = a+1; b < cons.size(); b++) {
				if (cons[a].polyline == cons[b].polyline) {
					// This is a double polyline traced from both ends.
					cons.erase(cons.begin() + b);
					cons.erase(cons.begin() + a);
					a--;
					break;
				}
			}
		}

		std::vector<Connection> shapeCons = { std::move(cons.back()) };
		cons.pop_back();

		// Retrace shapes.
		while (true) {
			if (shapeCons.front().StartPoint() == shapeCons.back().EndPoint()) {
				// Closed shape.
				mergedShapes.push_back(std::make_shared<Polyshape>());
				mergedShapes.back()->setConnections(shapeCons);
				mergedShapes.back()->setColor(colorArea);
				mergedShapes.back()->Cleanup();

				if (cons.empty())
					break;

				// Start a new shape.
				shapeCons = { std::move(cons.back()) };
				cons.pop_back();
			}
			for (int i = 0; i < cons.size(); i++)
			{
				if (shapeCons.back().EndPoint() == cons[i].StartPoint())
				{
					shapeCons.push_back(cons[i]);
					cons.erase(cons.begin() + i);
					break;
				}
			}
		}
	}
	
	SortShapes(mergedShapes);
	Export::SaveSVG(path, image_path, shape, mergedShapes);
}

// Snap endpoints of curves
void VectorGraphic::SnapEndpoints(const float& pSnappingDistance2)
{
	std::cout << "Snapping ";
	for (size_t i = 0; i < Polylines.size(); i++)
	{
		auto& polyline = Polylines[i];
		// Construct a new Vector with references to all the other polylines
		decltype(Polylines) otherLines(Polylines);
		otherLines.erase(otherLines.begin() + i);

		VE::Point start = polyline->Front();
		VE::Point end = polyline->Back();


		VE::Point closest;
		if (closestPointInRange(start, otherLines, closest, pSnappingDistance2)) {
			start = closest;
		}
		if (closestPointInRange(end, otherLines, closest, pSnappingDistance2)) {
			end = closest;
		}
		if (start != polyline->Front() || end != polyline->Back()) {
			std::vector<VE::Point> points = polyline->getPoints();
			points.front() = start;
			points.back() = end;
			polyline->setPoints(points);
			
			std::cout << "|";
		}
	}
	std::cout << " done.\n";
}


// trace in direction to determine overlap
// starting at lineA[a] == lineB[b]
// "end" is the last point which is the same for A and B
void TraceOverlap(const std::vector<VE::Point>& lineA, const std::vector<VE::Point>& lineB, int& a, int b) {
	int traceDir;
	int exitB;

	// trace B forwards (else backwards)
	if (lineB.size() != b + 1 && lineA[a + 1] == lineB[b + 1]) {
		traceDir = 1;
		exitB = lineB.size();
	}
	else if (-1 != b - 1 && lineA[a + 1] == lineB[b - 1]) {
		traceDir = -1;
		exitB = -1;
	}
	else {
		// only initial point is the same
		return;
	}

	while (true) {
		a++;
		b += traceDir;

		if (a == lineA.size() ||
			b == exitB ||
			lineA[a] != lineB[b])
		{
			break;
		}
	}
	// decrement a to get to the last working point
	a--;
	return;
}

// Params
// points		the path
// others		all the other polylines
// result		whatever remains from the path
void GetNoneOverlappingLines(const std::vector<VE::Point>& points,
	const std::vector<PolylinePtr>::iterator& othersBegin, const std::vector<PolylinePtr>::iterator& othersEnd,
	std::vector<PolylinePtr>& result)
{
	int start = 0;
	for (int i = 0; i < (int)points.size() - 1; i++)
	{
		// Compare to all the other polylines
		for (auto otherLine = othersBegin; otherLine != othersEnd; otherLine++)
		{
			int indexOther = (*otherLine)->PointIndex(points[i], 0);

			// Is there a same point from the otherLine?
			if (indexOther >= 0) {
				// Chop the first segments, if we aren't at the first point anymore,
				// and store it in the new polyline list.
				if (i > start) {
					//std::cout << "Chop at " << j << "/" << points.size() << "\n";
					std::vector<VE::Point> choppedPoints(points.begin() + start, points.begin() + i + 1);
					result.push_back(std::make_shared<VE::Polyline>(choppedPoints));
				}

				int end = i;
				// Trace to the "end" of the overlap by shifting i (~ the read head).
				TraceOverlap(points, (*otherLine)->getPoints(), end, indexOther);
				// Single point, but we had just progressed and chopped something off.
				// OR
				// There was a longer trace.
				// => These conditions are there to prevent getting stuck at the end of an overlap.
				if ((end == i && end > start) ||
					end > i)
				{
					start = end;
					i = end - 1;
					break;
				}
			}
		}
	}
	if (start < (int)points.size() - 1) {
		std::vector<VE::Point> choppedPoints(points.begin() + start, points.end());
		result.push_back(std::make_shared<VE::Polyline>(choppedPoints));
	}
	if (start != 0)
		std::cout << ".";
}

// Remove floats (Curves Segments which overlap)
// and split curves into separate segments at intersections
void VectorGraphic::RemoveOverlaps()
{
	int numberOfPolylines = Polylines.size();

	// The last polyline will not overlap, as all other have been corrected.
	for (int i = numberOfPolylines - 1; i > 0 - 1; i--)
	{
		// Take first out.
		VE::PolylinePtr polyline = Polylines[i];
		Polylines.erase(Polylines.begin() + i);

		const std::vector<VE::Point>& points = polyline->getPoints();

		decltype(Polylines) newPolylines;
		GetNoneOverlappingLines(points, Polylines.begin(), Polylines.begin() + i, newPolylines);

		Polylines.insert(Polylines.end(), newPolylines.begin(), newPolylines.end());
		std::cout << "|";
	}
	std::cout << "\n";
	for (auto& pl : Polylines)
		if (pl->Length() < 2)
			throw std::exception("WTF");
}

void VectorGraphic::MergeConnected(const float& pMinMergeAngle)
{
	std::cout << "Joining... ";
	for (int i = (int)Polylines.size() - 1; i >= 0; )
	{
		VE::PolylinePtr mainPolyline = Polylines[0];
		std::vector<Connection> connections;

		// create first item
		std::vector<Connection> nextConnections;
		nextConnections.push_back(Connection());
		nextConnections[0].at = Connection::Location::start;
		nextConnections[0].polyline = mainPolyline;
		Point LoopConnect = mainPolyline->Front();

		auto otherLines = Polylines;
		while (true) {
			// Add the con(nection) to connections.
			Connection con = nextConnections[0];
			connections.push_back(con);
			VE::PolylinePtr  polyline = con.polyline;

			// Remove the item from the otherLines
			otherLines.erase(std::find(otherLines.begin(), otherLines.end(), con.polyline));

			// Determine the endPoint for the next search.
			Point searchPoint = con.EndPoint();
			if (searchPoint == LoopConnect) break;

			// Do the search.
			nextConnections = GetConnections(searchPoint, otherLines);
			if (nextConnections.size() != 1) break;
			// Compare angles, merge short lines (<4 points) regardless.
			Connection& next = nextConnections[0];
			if (GetConnectionAngle(con, next) < pMinMergeAngle
				&& con.polyline->Length() >= 4
				&& next.polyline->Length() >= 4) {
				break;
			}
		}

		// Remove the first (main) element and do the search backwards.
		nextConnections = std::vector<Connection>();
		nextConnections.push_back(Connection());
		nextConnections[0].at = Connection::Location::start;
		nextConnections[0].polyline = connections[0].polyline;
		LoopConnect = connections.back().EndPoint();

		connections.erase(connections.begin());

		otherLines = Polylines;
		while (true) {
			Connection con = nextConnections[0];
			connections.insert(connections.begin(), con);

			VE::PolylinePtr  polyline = con.polyline;

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
			if (GetConnectionAngle(next, con) < pMinMergeAngle
				&& con.polyline->Length() >= 4
				&& next.polyline->Length() >= 4) {
				break;
			}
		}

		// Finally, check if we have Connections and if yes
		// remove them from the main list.
		if (connections.size() > 1) {

			// Contruct a new Point Sequence
			std::vector<Point> points;

			for (Connection& con: connections)
			{
				const std::vector<Point>& newPoints = con.polyline->getPoints();
				if (con.at == Connection::Location::start) {
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
			VE::PolylinePtr newPolyline = std::make_shared<VE::Polyline>();
			newPolyline->setPoints(points);
			Polylines.push_back(newPolyline);

			std::cout << connections.size() << ", ";
		}
		else {
			std::rotate(Polylines.begin(), Polylines.begin() + 1, Polylines.end());
		}
		// Decrease i by as many items as we remove (connections.size())).
		i -= connections.size();
	}
	std::cout << " done.\n";
}

void VectorGraphic::ComputeConnectionStatus()
{
	for (VE::PolylinePtr& pl:Polylines)
	{
		auto otherLines = Polylines;
		otherLines.erase(std::find(otherLines.begin(), otherLines.end(), pl));

		// compute front
		pl->ConnectFront = GetConnections(pl->Front(), otherLines);
		pl->ConnectBack = GetConnections(pl->Back(), otherLines);
		pl->UpdateStatus();
	}
}

void VectorGraphic::RemoveUnusedConnections()
{
	int amount = 0;
	while (amount != Polylines.size()) {
		amount = Polylines.size();
		
		decltype(Polylines) validLines;
		for (VE::PolylinePtr& pl : Polylines)
		{
			if (pl->Status() != Polyline::LineStat::invalid)
				validLines.push_back(pl);
		}

		Polylines = std::move(validLines);

		ComputeConnectionStatus();
	}

}

void VectorGraphic::Split(VE::PolylinePtr pl, VE::Point pt)
{
	auto& orig = pl->getPoints();
	std::vector<VE::Point> front, back;

	auto it = std::find(orig.begin(), orig.end(), pt);

	// edgy cases
	if (it == orig.begin() ||
		it == orig.end() - 1) {
		return;
	}
	if (it == orig.end()) {
		throw std::invalid_argument("Point not on polyline.");
	}

	front.insert(front.end(), orig.begin(), it + 1);
	back.insert(back.end(), it, orig.end());

	Polylines.erase(std::find(Polylines.begin(), Polylines.end(), pl));
	DeleteConnections(pl);
	Polylines.push_back(std::make_shared<VE::Polyline>(front));
	Polylines.push_back(std::make_shared<VE::Polyline>(back));
}

void VectorGraphic::Connect(VE::Connection& a, VE::Connection& b)
{
	std::vector<VE::Point> pts;
	pts.push_back(a.StartPoint());
	pts.push_back(b.StartPoint());

	PolylinePtr pl = std::make_shared<VE::Polyline>(pts);

	pl->ConnectFront = GetConnections(pl->Front(), Polylines);
	pl->ConnectBack = GetConnections(pl->Back(), Polylines);
	pl->UpdateStatus();

	// Update the connected lines only
	std::vector<PolylinePtr> updatableLines;
	for (auto& c : pl->ConnectFront) updatableLines.push_back(c.polyline);
	for (auto& c : pl->ConnectBack) updatableLines.push_back(c.polyline);


	for (VE::PolylinePtr& line : updatableLines)
	{
		auto otherLines = Polylines;
		otherLines.erase(std::find(otherLines.begin(), otherLines.end(), line));

		// TODO only compute necessary end
		line->ConnectFront = GetConnections(line->Front(), otherLines);
		line->ConnectBack = GetConnections(line->Back(), otherLines);
		line->UpdateStatus();
	}

	Polylines.push_back(pl);
}

void VectorGraphic::Delete(VE::PolylinePtr line)
{
	Polylines.erase(std::find(Polylines.begin(), Polylines.end(), line));
	DeleteConnections(line);
}

// Traces a single shape, starting from connections.back().
// Removes the connections of the shape on the way. If the shape is
// invalid, a nullptr is returned.
void VectorGraphic::TraceSingleShape(std::vector<VE::Connection> & connections, VE::PolyshapePtr& result)
{
	result = nullptr;

	std::vector<Connection> shapeCons;
	std::vector<Connection> potentialConnections;

	shapeCons.push_back(std::move(connections.back()));
	connections.pop_back();

	// Get the loop out first.
	auto& pl = shapeCons[0].polyline;
	if (pl->Back() == pl->Front()) {
		goto TraceSingleShape_Finalize;
	}

	// Trace the connections for shapes with multiple components.
	do {
		const VE::Point& currentEndPoint = shapeCons.back().EndPoint();

		// Populate the PotentialConnections.
		potentialConnections.clear();
		for (auto& potCon : connections)
		{
			
			if (potCon.StartPoint() == currentEndPoint &&
				// Don't allow loops
				potCon.polyline->Back() != potCon.polyline->Front())
			{
				bool already_used_polyline = false;
				for (auto&shapeCon:shapeCons)
					if (shapeCon.polyline == potCon.polyline) {
						already_used_polyline = true;
						break;
					}
				if (!already_used_polyline)
					potentialConnections.push_back(potCon);
			}
		}

		// Sort the potential connections from left-to-right and add the
		// best match to the shapeCons and remove it from all the "connections".
		if (!potentialConnections.empty()) {
			shapeCons.back().SortOther(potentialConnections);
			auto& bestMatch = potentialConnections[0];

			auto item = std::find(connections.begin(), connections.end(), bestMatch);
			shapeCons.push_back(*item);
			connections.erase(item);
		}

		// Check if we can close the shape.
		if (shapeCons.front().StartPoint() == shapeCons.back().EndPoint()) {
			goto TraceSingleShape_Finalize;
		}
	} while (!potentialConnections.empty());

	return;

TraceSingleShape_Finalize:

	// Calculate angles to verify the shape.
	auto shape = std::make_shared<Polyshape>();

	shape->setConnections(shapeCons);

	// A positive angle means correct way around.
	if (shape->CounterClockwise()) {
		shape->Cleanup();
		result = shape;
	}
	else {
		std::cout << "Shape is inverted.\n";
	}
}

void VectorGraphic::CalcShapes()
{
	Polyshapes.clear();

	// Calculate the shapes with multiple connections.
	std::vector<Connection> allCons;
	for (auto& pl : Polylines) {
		allCons.push_back(Connection(pl, Connection::Location::start));
		allCons.push_back(Connection(pl, Connection::Location::end));
	}
	for (auto& pl : Polylines)
		if (pl->Length() < 2)
			throw std::exception("WTF");

	while (!allCons.empty()) {
		VE::PolyshapePtr shape;
		TraceSingleShape(allCons, shape);
		if (shape != nullptr)
			Polyshapes.push_back(shape);
	}
}

void VectorGraphic::ColorShapesRandom()
{
	const std::vector<cv::Scalar> coolColors = { cv::Scalar(240,163,255),cv::Scalar(0,117,220), cv::Scalar(153,63,0), cv::Scalar(76,0,92), cv::Scalar(25,25,25), cv::Scalar(0,92,49), cv::Scalar(43,206,72), cv::Scalar(255,204,153), cv::Scalar(128,128,128), cv::Scalar(148,255,181), cv::Scalar(143,124,0), cv::Scalar(157,204,0), cv::Scalar(194,0,136), cv::Scalar(0,51,128), cv::Scalar(255,164,5), cv::Scalar(255,168,187), cv::Scalar(66,102,0), cv::Scalar(255,0,16), cv::Scalar(94,241,242), cv::Scalar(0,153,143), cv::Scalar(224,255,102), cv::Scalar(116,10,255), cv::Scalar(153,0,0), cv::Scalar(255,255,128), cv::Scalar(255,255,0), cv::Scalar(255,80,5) };
	auto color = coolColors.begin();
	for (auto&shape:Polyshapes)
	{
		shape->getColor()->Color = *color;
		if (++color == coolColors.end())
			color = coolColors.begin();
	}
}

// Sort the polyshapes, placing smaller ones in bigger ones for correct drawing.
void VectorGraphic::SortShapes()
{
	SortShapes(Polyshapes);
}

void VectorGraphic::SortShapes(std::vector<PolyshapePtr>& shapes)
{
	// Sort shapes => small shapes have to come last to not be
	// covered by the bigger shapes.
	for (int a = 0; a < (int)shapes.size() - 1; a++)
	{
		auto& shapeA = shapes[a];

		for (auto b = a + 1; b < shapes.size(); b++)
		{
			auto& shapeB = shapes[b];

			auto& boundsA = shapeA->getBounds();
			auto& boundsB = shapeB->getBounds();

			if (boundsB.Contains(boundsA)) {
				shapes.insert(shapes.begin() + b + 1, shapeA);
				shapes.erase(shapes.begin() + a);
				a--;
				b = (int)shapes.size() - 1;
			}
		}
	}
}

void VectorGraphic::ClearShapes()
{
	Polyshapes.clear();
}

// Creates a new shape by getting the Polyline, left to the target
// point, and tracing the connections. Returns a nullptr if there
// is tracable outline.
VE::PolyshapePtr VectorGraphic::CreateShape(const VE::Point & target)
{
	// Find closest left Polyline, with none invalid status.
	VE::PolyshapePtr shape = std::make_shared<Polyshape>();


	VE::Connection conResult;
	if (!ClosestConnectionLeft(target, Polylines, conResult)) {
		return nullptr;
	}

	std::vector<Connection> allCons;
	for (auto& pl : Polylines) {
		allCons.push_back(Connection(pl, Connection::Location::start));
		allCons.push_back(Connection(pl, Connection::Location::end));
	}

	allCons.erase(std::find(allCons.begin(), allCons.end(), conResult));

	allCons.push_back(conResult);

	VE::PolyshapePtr ptr = nullptr;
	TraceSingleShape(allCons, ptr);

	return ptr;
}

VE::PolyshapePtr VectorGraphic::ColorShape(const VE::Point& pt, VE::ColorAreaPtr& color)
{
	// Create new shape, there could be a larger shape in which we
	// click otherwise.
	VE::PolyshapePtr shape = CreateShape(pt);
	
	if (shape == nullptr) return shape;

	auto& shapeCon0 = shape->getConnections()[0];
	bool found = false;

	for (auto&ps:Polyshapes)
	{
		for (auto&con:ps->getConnections())
		{
			if (VE::Connection::Cmp(con, shapeCon0)) {
				shape = ps;
				found = true;
				break;
			}
		}
	}
	if (!found) {
		Polyshapes.push_back(shape);
		SortShapes();
	}

	if (shape != nullptr) {
		shape->setColor(color);
	}

	return shape;
}

void VectorGraphic::PickColor(const VE::Point& pt, VE::ColorAreaPtr& color)
{
	VE::PolyshapePtr shape;
	ClosestPolyshape(pt, shape);

	if (shape != nullptr) {
		color = shape->getColor();
	}
}

bool VectorGraphic::DeleteShape(const VE::Point& pt)
{
	VE::PolyshapePtr shape = nullptr;
	ClosestPolyshape(pt, shape);

	if (shape != nullptr) {
		auto it = std::find(Polyshapes.begin(), Polyshapes.end(), shape);
		Polyshapes.erase(it);
		return true;
	}
	return false;
}

void VectorGraphic::ClosestPolyline(cv::Mat& img, VE::Transform& t, float& distance2, const VE::Point& pt,
	VE::Point& closest, VE::PolylinePtr& element)
{
	Bounds bounds(0, 0, img.cols, img.rows);
	t.applyInv(bounds);
	ClosestPolyline(bounds, distance2, pt, closest, element);
}

void VectorGraphic::ClosestPolyline(VE::Bounds& bounds, float& distance2, const VE::Point& pt,
	VE::Point& closest, VE::PolylinePtr& element)
{
	int id = 0,
		closest_id = 0;
	for (auto&pl:Polylines) {
		// Check for visibility first.
		if (pl->AnyPointInRect(bounds)) {
			float previous = distance2;
			pl->Closest2(pt, distance2, closest);
			if (previous != distance2) {
				element = pl;
				closest_id = id;
			}
		}
		id++;
	}
}

bool ClosestPolylineLeft1(const VE::Point& target, VE::PolylinePtr& line, float& maxDist, bool&downwards)
{
	const float origMaxDist = maxDist;
	// Check the the polyline's bound will hold a left collision.
	auto& plBounds = line->getBounds();
	if (plBounds.x0 >= target.x
		|| target.x - plBounds.x1 > maxDist
		|| plBounds.y0 >= target.y
		|| plBounds.y1 <= target.y) {
		return false;
	}

	float maxLen = line->getMaxLength();
	float x_max = target.x + maxLen;
	float y_min = target.y - maxLen;
	float y_max = target.y + maxLen;

	auto& pts = line->getPoints();

	for (auto pt = pts.begin(); pt!= pts.end() - 1; pt ++)
	{
		// Check if in y-range.
		if ((*pt).x >= x_max ||
			(*pt).y <= y_min ||
			(*pt).y >= y_max) {
			continue;
		}
		bool pt1_below = (*pt).y <= target.y;
		auto& next = *(pt + 1);
		bool pt2_below = (*(pt + 1)).y <= target.y;
		if (pt1_below == pt2_below) {
			continue;
		}


		// Do a simple linear intersection, which should be inbetween
		// the two points.
		float m = (target.y - (*pt).y) / ((*(pt + 1)).y - (*pt).y);
		assert(m >= 0 && m <= 1);

		float x = (*pt).x + m * ((*(pt + 1)).x - (*pt).x);
		if (target.x > x) {
			float newDist = target.x - x;
			if (newDist < maxDist) {
				maxDist = newDist;
				downwards = pt1_below;
			}
		}
	}
	return maxDist < origMaxDist;
}


bool VectorGraphic::ClosestConnectionLeft(const VE::Point& target, std::vector<VE::PolylinePtr>& lines, VE::Connection& result)
{
	VE::PolylinePtr closest = nullptr;
	bool downwards;
	float maxDist = VE::FMAX;

	for (auto& line : lines) {
		if (ClosestPolylineLeft1(target, line, maxDist, downwards)) {
			closest = line;
		}
	}
	if (closest == nullptr)	{
		return false;
	}
	else {
		result = Connection(closest, VE::Connection::Location::start);
		if (!downwards)
			result.at = VE::Connection::Location::end;
		return true;
	}
}

// For interactive picking.
// Get the left-closest intersection.
void VectorGraphic::ClosestPolyshape(const VE::Point& target, VE::PolyshapePtr& element)
{
	element = nullptr;

	// Make a list of all relevant lines used in the Polyshapes.
	std::vector<VE::PolylinePtr> lines;
	std::vector<VE::PolyshapePtr> shapes;

	for (auto ps = Polyshapes.begin(); ps != Polyshapes.end(); ps++)
	{
		auto& elBounds = (*ps)->getBounds();
		if (!(*ps)->getBounds().Contains(target)) {
			continue;
		}

		shapes.push_back(*ps);
		for (auto& con : (*ps)->getConnections())
		{
			if (std::find(lines.begin(), lines.end(), con.polyline) == lines.end())
				lines.push_back(con.polyline);
		}
	}

	VE::Connection conResult;
	if (!ClosestConnectionLeft(target, lines, conResult)) {
		return;
	}

	for (auto ps = shapes.rbegin(); ps != shapes.rend(); ps++) {
		for (auto& con : (*ps)->getConnections()) {
			if (VE::Connection::Cmp(con, conResult)) {
				element = (*ps);
				return;
			}
		}
	}
	
	// If we reach this position, then we have clicked in a bounding box,
	// but not inside a polyshape.
}

inline void ClosestEndPoint_TryReplace(float& maxDist2, const VE::Point& target, const VE::Point& candidate, VE::Point& closest) {
	VE::Point dir = candidate - target;
	float dist2 = dir.x * dir.x + dir.y * dir.y;
	if (dist2 < maxDist2) {
		maxDist2 = dist2;
		closest = candidate;
	}
}

void VectorGraphic::ClosestEndPoint(cv::Mat& img, VE::Transform& t, float& maxDist2, const VE::Point& pt, VE::Point& closest)
{
	Bounds bounds(0, 0, img.cols, img.rows);
	t.applyInv(bounds);
	ClosestEndPoint(bounds, maxDist2, pt, closest);
}

void VectorGraphic::ClosestEndPoint(VE::Bounds& b, float & maxDist2, const VE::Point& pt, VE::Point& closest)
{
	for (auto&pl:Polylines)
	{
		if (b.Overlap(pl->getBounds())) {
			ClosestEndPoint_TryReplace(maxDist2, pt, pl->Front(), closest);
			ClosestEndPoint_TryReplace(maxDist2, pt, pl->Back(), closest);
		}
	}
}

void VectorGraphic::Draw(cv::Mat& img, VE::Transform& t)
{
	Bounds bounds(0, 0, img.cols, img.rows);
	t.applyInv(bounds);

	for (auto el = Polyshapes.begin(); el != Polyshapes.end(); el++) {
		
		if ((*el)->AnyPointInRect(bounds)) {
			(*el)->Draw(img, t);
		}

	}

	for (auto el = Polylines.begin(); el != Polylines.end(); el++) {
		if ((*el)->AnyPointInRect(bounds)) {
			(*el)->Draw(img, t);
		}
	}
}

VE::Bounds VectorGraphic::getBounds()
{
	Bounds b = Polylines[0]->getBounds();
	for (auto&pl:Polylines)
	{
		b.x0 = std::min(b.x0, pl->getBounds().x0);
		b.x1 = std::max(b.x1, pl->getBounds().x1);
		b.y0 = std::min(b.y0, pl->getBounds().y0);
		b.y1 = std::max(b.y1, pl->getBounds().y1);
	}
	return b;
}
