#include "VectorGraphic.h"

#include "Polyline.h"
#include "Polyshape.h"
#include "Export.h"

#include <iostream>
#include <map>
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

		if (polyline->Front() == polyline->Back() 
			|| polyline->Status() == Polyline::LineStat::invalid) {
			// Don't use loops or invalid lines.
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



const float VectorGraphic::MERGE_DISTANCE = 1e-4;
const float VectorGraphic::MERGE_DISTANCE2 = 1e-8;


const char* quotationMark = "\"'";
const char* whitespace = "\t ";
const char* alphabetic = "abcdefghijklmnopqrstuvwxyz";
const char* numeric = "-0123456789";
const char* numericDot = "-0123456789.";
void parseXML(std::map<std::string, std::string>& attributes, std::string& line) {
	attributes.clear();

	if (line.length() == 0) return;
	size_t tag_open = line.find("<");
	size_t tagName_start = line.find_first_of(alphabetic, tag_open);
	size_t tagName_end = line.find_first_not_of(alphabetic, tagName_start);
	size_t tag_close = line.find(">", tagName_end);
	if (tag_open == std::string::npos || tag_close == std::string::npos || 
		tagName_start == std::string::npos || tagName_end == std::string::npos)
		return;

	attributes.insert(std::make_pair(std::string("tag"), line.substr(tagName_start, tagName_end - tagName_start)));
	
	size_t nextAttr = tagName_end;
	while(nextAttr < line.size()) {
		if (line.find_first_of(whitespace, nextAttr, 1) == nextAttr) return;
		size_t attr_start = line.find_first_of(alphabetic, nextAttr);
		size_t attr_end = line.find_first_not_of(alphabetic, attr_start);
		size_t equals_sign = line.find("=", attr_end);
		size_t content_start = line.find_first_of(quotationMark, equals_sign);
		if (content_start == std::string::npos)
			return;
		std::string delimiter = line.substr(content_start, 1);
		size_t content_end = line.find(delimiter, content_start + 1);

		if (content_end == std::string::npos)
			return;
		attributes.insert(std::make_pair(
			line.substr(attr_start, attr_end - attr_start),
			line.substr(content_start + 1, content_end - content_start - 1)
		));
		nextAttr = content_end + 1;
	}
}

void parseFloatPairs(std::vector<VE::Point>& points, std::string& line) {
	points.clear();

	size_t pair_start = 0;
	while (true) {
		pair_start = line.find_first_not_of(whitespace, pair_start);
		size_t pair_comma = line.find(",", pair_start);
		size_t pair_end = line.find_first_of(whitespace, pair_comma);
		if (pair_end == std::string::npos)
			return;

		VE::Point pt;
		pt.x = std::atof(line.substr(pair_start, pair_comma - pair_start).c_str());
		pt.y = std::atof(line.substr(pair_comma + 1, pair_end - pair_comma - 1).c_str());
		points.push_back(pt);
		pair_start = pair_end + 1;
	}
}

void parsePath(std::vector<VE::Point>& points, std::string& path) {
	points.clear();

	size_t cursor = 0;
	// Verify M, parse first point, store as endpoint
	cursor = path.find_first_of('M', cursor) + 1;
	size_t num1Start = path.find_first_of(numericDot, cursor);
	size_t num1End = path.find_first_not_of(numericDot, num1Start);
	size_t num2Start = path.find_first_of(numericDot, num1End);
	size_t num2End = path.find_first_not_of(numericDot, num2Start);
	VE::Point endpoint;
	endpoint.x = std::atof(path.substr(num1Start, num1End - num1Start).c_str());
	endpoint.y = std::atof(path.substr(num2Start, num2End - num2Start).c_str());
	points.push_back(endpoint);
	cursor = num2End;

	while (true) {
		// Verify Q, parse 2, sample points and add, store endpoint
		cursor = path.find_first_of('Q', cursor) + 1;
		if (cursor == std::string::npos + 1)
			return;
			
		size_t num1 = path.find_first_of(numericDot, cursor);
		size_t num2 = path.find_first_of(',', num1) + 1;
		size_t num3 = path.find_first_of(',', num2) + 1;
		size_t num4 = path.find_first_of(',', num3) + 1;
		size_t num4End = path.find_first_of(whitespace, num4);
		VE::Point pt0 = endpoint;
		VE::Point pt1(	std::atof(path.substr(num1, num2 - 1 - num1).c_str()),
						std::atof(path.substr(num2, num3 - 1 - num2).c_str())	);
		VE::Point pt2(	std::atof(path.substr(num3, num4 - 1 - num3).c_str()),
						std::atof(path.substr(num4, num4End  - num4).c_str())	);


		// samples quadratic bezier
		for (float i = 0; i < 20; i++)
		{
			float t = (i + 1) / 40;
			float t_inv = 1 - t;
			float t_inv2 = t_inv * t_inv;
			VE::Point pt = t_inv2 * pt0
				+ 2 * t * t_inv * pt1
				+ t * t * pt;
			points.push_back(pt);
		}
		endpoint = pt2;
	}
}

void parseIntPairs(std::vector<std::pair<int, int>>& pairs, std::string& line) {
	pairs.clear();
	size_t pair_start = 0;
	while (true) {
		pair_start = line.find_first_not_of(whitespace, pair_start);
		size_t pair_comma = line.find(",", pair_start);
		size_t pair_end = line.find_first_of(whitespace, pair_comma);
		if (pair_end == std::string::npos)
			return;

		int a = std::atoi(line.substr(pair_start, pair_comma - pair_start).c_str());
		int b = std::atoi(line.substr(pair_comma + 1, pair_end - pair_comma - 1).c_str());
		pairs.push_back(std::pair<int, int>(a, b));
		pair_start = pair_end + 1;
	}
}

void parseIntChain(std::vector<int>& chain, std::string& line) {
	chain.clear();
	size_t int_start = 0;
	while (true) {
		int_start = line.find_first_of(numeric, int_start);
		size_t int_end = line.find_first_not_of(numeric, int_start);

		if (int_start == std::string::npos) {
			return;
		}
		if (int_end == std::string::npos) {
			int_end = line.size();
		}

		int a = std::atoi(line.substr(int_start, int_end - int_start).c_str());
		chain.push_back(a);

		int_start = int_end;
	}
}

void VectorGraphic::AddPolyline(const std::vector<VE::Point>& pts)
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
	if (!myfile.is_open())
		return;
	int i = 0;
	while (getline(myfile, line))
	{
		std::map<std::string, std::string> attributes;
		parseXML(attributes, line);
		auto tagName = attributes.find("tag");
		if (tagName == attributes.end())
			continue;

		if (tagName->second == "polyline") {
			std::vector<VE::Point> pts;
			auto pointIt = attributes.find("points");
			if (pointIt != attributes.end()) {
				parseFloatPairs(pts, pointIt->second);
			}
			if (pts.size() > 1) {
				AddPolyline(pts);
			}
			else {
				std::cout << "Polyline too short.";
			}
		}
		else if (tagName->second == "path") {
			// fixed sampling rate for quadratic curves
			std::vector<VE::Point> pts;
			auto pointIt = attributes.find("d");
			if (pointIt != attributes.end()) {
				parsePath(pts, pointIt->second);
			}
			if (pts.size() > 1) {
				AddPolyline(pts);
			}
			else {
				std::cout << "Path went wrong.";
			}
		}
		else if (tagName->second == "shape") {
			if (attributes.find("data") != attributes.end() &&
				attributes.find("name") != attributes.end() &&
				attributes.find("color") != attributes.end()) {

				VE::PolyshapeData shapeData;
				parseIntPairs(shapeData.data, attributes.find("data")->second);
				std::vector<int> colors;
				std::string colorString = attributes.find("color")->second;
				size_t colorStart = colorString.find("(") + 1;
				size_t colorEnd = colorString.find(")");

				parseIntChain(colors, colorString.substr(colorStart, colorEnd - colorStart));
				shapeData.colorArea = std::make_shared<ColorArea>();
				shapeData.colorArea->Color = cv::Scalar(colors[0], colors[1], colors[2]);
				shapeData.colorArea->Name = attributes.find("name")->second;

				VE::PolyshapePtr shape = Polyshape::FromData(Polylines, shapeData);
				if (shape == nullptr) {
					std::cout << "Failed to make shape: " << shapeData.colorArea->Name << "\n";
				}
				else {
					Polyshapes.push_back(shape);
				}
			}
			else {
				std::cout << "Invalid shape.\n";
			}
		}

	}
	myfile.close();
	MakeColorsUnique();
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
			shapeData.colorArea = ps->getColor();
			shapeDatas.push_back(shapeData);
		}
	}
	Export::SaveSVG(path, image_path, shape, Polylines, shapeDatas);
}

// Saves the polyshapes, mergeing shapes of the same ColorArea prior.
// Also extract the smaller shapes from larger ones which contain them.
void VectorGraphic::SavePolyshapes(std::string path, std::string image_path, cv::Size2i shape)
{
	std::cout << "Exporting shapes ";
	// Merge same ColorArea shapes.
	auto saveShapes = Polyshapes;
	decltype(Polyshapes) newShapes;

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
			
			if (*colorArea == *shapeB->getColor())
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

		decltype(Polyshapes) mergedShapes;
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


		// Check the mergedShapes for overlaps before inserting them 
		// into the newShapes.
		SortShapes(mergedShapes);
		std::vector<VE::Connection> otherCons;
		std::vector<VE::PolylinePtr> otherLines;

		for (size_t i = 1; i < mergedShapes.size(); i++)
		{
			bool doContainCheck = false;
			for (size_t j = 0; j < i; j++) {
				if (mergedShapes[j]->getBounds().Contains(mergedShapes[i]->getBounds())) {
					doContainCheck = true;
					break;
				}
			}

			for (auto& con : mergedShapes[i - 1]->getConnections())
			{
				otherCons.push_back(con);
				otherLines.push_back(con.polyline);
			}

			if (!doContainCheck) {
				continue;
			}
			VE::Connection firstCon = mergedShapes[i]->getConnections()[0];
			VE::Connection conResult;
			if (ClosestConnectionLeft(firstCon.StartPoint(), otherLines, conResult)) {
				for (auto& con : otherCons)
				{
					if (con.polyline == conResult.polyline)
					{
						if (con.at == conResult.at) {
							//std::cout << "Detected enclosed shape: " << mergedShapes[i]->getColor()->Name << "\n";
							mergedShapes.erase(mergedShapes.begin() + i);
							i--;
						}
						else {
							// Shapes are next to each other, and that's ok.
						}
						// We can break. There will be no other connection with the same
						// polyline, since we removed the doubles.
						break;
					}
				}
			}
		}

		newShapes.insert(newShapes.end(), mergedShapes.begin(), mergedShapes.end());

		std::cout << "|";
	}
	
	SortShapes(newShapes);
	std::cout << "\nSaving.\n"; 
	Export::SaveSVG(path, image_path, shape, newShapes);
}

// Snap endpoints of curves
void VectorGraphic::SnapEndpoints(const float& pSnappingDistance2)
{
	std::cout << "Snapping ";
	for (size_t i = 0; i < Polylines.size(); i++)
	{
		VE::PolylinePtr polyline = Polylines[i];
		// Construct a new Vector with references to all the other polylines
		std::vector<VE::PolylinePtr> otherLines(Polylines);
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


void LinesWithPaddedBounds(const VE::PolylinePtr& activeLine,
	const std::vector<PolylinePtr>& otherLines,
	std::vector<PolylinePtr>& othersSelected, std::vector<Bounds>& paddedBounds, std::vector<float>& distances2)
{
	othersSelected.clear();
	paddedBounds.clear();
	distances2.clear();

	for (auto other = otherLines.begin(); other != otherLines.end(); other++)
	{
		if (!activeLine->getBounds().Overlap((*other)->getBounds()))
			continue;
		othersSelected.push_back(*other);
		Bounds bounds = (*other)->getBounds();
		bounds.Pad(activeLine->getMaxLength());
		paddedBounds.push_back(bounds);

		float maxLen = activeLine->getMaxLength();
		float maxLenOther = (*other)->getMaxLength();
		float distanceSum2 = maxLen * maxLen // Can't half the initial value, because the last segment can be split at it's end.
			+ maxLenOther * maxLenOther / 4
			+ 3 * FLT_EPSILON;
		distances2.push_back(distanceSum2);
	}
}

// Check if a dot product is within few degrees (1°) of a zero angle.
inline bool AngleIsZero(const float& dotProduct) { return dotProduct >= 0.9998f; }

// Use this function for case 1x when a congruent point is found. (Case 1)
// Functions checks if line is parallel and if a segment can be (partially) skipped.
// It would replace the skipTo [t, pt].
inline void SkipSplit_PointIntersect(const VE::Point& a1, const VE::Point& a2, const VE::Point& b2,
	float& skipToAt, VE::Point& skipToPt)
{
	const VE::Point& aDir = a2 - a1;
	const VE::Point& bDir = b2 - a1;
	const float& aMag = VE::Magnitude(aDir);
	const float& bMag = VE::Magnitude(bDir);

	const float angle = VE::Dot(aDir, bDir) / (aMag * bMag);
	if (AngleIsZero(angle)) {
		const float t = bMag / aMag;
		if (t > skipToAt) {
			skipToAt = t;
			skipToPt = b2;
		}
	}
}

// Use this function for case 1x when NO congruent point is found. (Case 2 & 3)
inline void SkipSplit_ClosePoints(const VE::Point& a1, const VE::Point& a2, const VE::Point& b1, const VE::Point& b2,
	bool& startSplit, float& splitAt, VE::Point& splitPt, float& skipToAt, VE::Point& skipToPt,
	const float& MERGE_DISTANCE, const float& MERGE_DISTANCE2) {
	const VE::Point& aDir = a2 - a1;
	const VE::Point& bDir = b2 - b1;
	const float& aMag2 = VE::Magnitude2(aDir);
	const float& bMag2 = VE::Magnitude2(bDir);

	float MD_t = MERGE_DISTANCE / std::sqrt(aMag2);
	float MD_u = MERGE_DISTANCE / std::sqrt(bMag2);

	const float angle = VE::Dot(aDir, bDir) / (std::sqrt(aMag2) * std::sqrt(bMag2));
	// Check if the lines are parallel.
	if (AngleIsZero(std::abs(angle))) {
		// Check Distance from "other" point to current line segment.
		float t1 = (b1 - a1).dot(aDir) / aMag2;
		const VE::Point closest_b1 = a1 + t1 * aDir;
		const float dist2_b1 = VE::Magnitude2(b1 - closest_b1);

		if (dist2_b1 > MERGE_DISTANCE2)
			return;

		float t2 = (b2 - a1).dot(aDir) / aMag2;
		
		bool swap = false;
		if (t1 > t2) {
			float tmp = t2;
			t2 = t1;
			t1 = t2;
			swap = true;
		}

		if (t2 < 0 || t1 >= 1) return;
		if (t1 <= MD_t) {
			startSplit = true;
			if (t2 > skipToAt) {
				skipToAt = t2;
				skipToPt = swap ? b1 : b2;
			}
		}
		else {
			if (t1 < splitAt) {
				splitAt = t1;
				splitPt = swap ? b2 : b1;
			}
		}
	}
	else {
		// Since the lines are not parallel, if we are already skipping,
		// then there is no need to check for splitting.
		if (skipToAt > 0.f) return;


		float rxs = VE::Cross(aDir, bDir);
		const VE::Point ab = b1 - a1;
		float t = VE::Cross(ab, bDir) / rxs;
		if (t < MD_t || t + MD_t >= 1) return;
		float u = VE::Cross(ab, aDir) / rxs;
		if (u < 0 || u > 1) return;
		if (t == 0) {
			startSplit = true;
		}
		else {

			if (t < splitAt) {
				if (u <= MD_u) {
					splitAt = t;
					splitPt = b1;
				}
				else if (u >= 1 - MD_u) {
					splitAt = t;
					splitPt = b2;
				}
				else {
					VE::Point possibleSplitPt = b1 + u * bDir;
					// Prevent rounding errors, even if t != 0 | 1, b1 + u*bDir may just be a1 or a2.
					if (possibleSplitPt != a1 && possibleSplitPt != a2) {
						splitAt = t;
						splitPt = possibleSplitPt;
					}
				}
			}
		}
	}
}

inline void Intersection(const VE::Point& a, const VE::Point& aDir, const VE::Point& b, const VE::Point& bDir, float& t, float& u) {
	float rxs = VE::Cross(aDir, bDir);
	const VE::Point ab = b - a;

	// Lines may not be collinear, otherwise it is a 0 division.
	t = VE::Cross(ab, aDir) / rxs;
	u = VE::Cross(ab, bDir) / rxs;
}

// Params
// points		this line gets destroyed
// others		all the other polylines which bounds intersect (!)
// result		whatever segments remain from the path
/*
	There are several cases, which can occur simultaneously.

	1a	Point Point Intersection
	1b	Point Point Intersection with parallel, shorter line 
	1c	Point Point Intersection with parallel, longer line 
	2a	Point Line Intersection
	2b	Point Line Intersection with a parallel, shorter line (it ends earlier)
	2b	Point Line Intersection with a parallel, longer line (it ends later)
	3a	Line Point Intersection
	3b	Line Point Intersection with a parallel line (shorter or longer)
	3c	Line Line Intersection
*/
void VectorGraphic::GetNoneOverlappingLines(std::vector<VE::Point>& points, std::vector<VE::PolylinePtr>& result,
	const std::vector<VE::PolylinePtr> others, const std::vector<VE::Bounds> paddedBounds, const std::vector<float> distances2)
{
	result.clear();
	int start = 0;
	// Only iterate up to the penultimate point.
	for (int i = 0; i < (int)points.size() - 1; i++)
	{
		VE::Point& pt = points[i];
		VE::Point& ptNext = points[i + 1];
		const VE::Point ptDir = ptNext - pt;
		const float dirPtMag2 = VE::Magnitude(ptDir);

		bool startSplit = false;
		float splitAt = 1.f;
		VE::Point splitPt = ptNext;
		float skipToAt = 0.f;
		VE::Point skipToPt = ptNext;

		// Compare to all the other polylines.
		for (int j = 0; j < others.size(); j++)
		{
			if (!paddedBounds[j].Contains(pt))
				continue;

			VE::PolylinePtr otherLine = others[j];
			int indexOther = otherLine->PointIndex(pt, distances2[j]);
			if (indexOther < 0)
				continue;

			const std::vector<VE::Point>& otherPoints = otherLine->getPoints();

			// 1. Point Point Intersection
			if (points[i] == otherPoints[indexOther]) {
				startSplit = true;
				if (indexOther < otherPoints.size() - 1)
					SkipSplit_PointIntersect(pt, ptNext, otherPoints[indexOther + 1], skipToAt, skipToPt);
				if (indexOther > 0)
					SkipSplit_PointIntersect(pt, ptNext, otherPoints[indexOther - 1], skipToAt, skipToPt);
			}
			// 2. Other Intersection
			else {
				if (indexOther < otherPoints.size() - 1)
					SkipSplit_ClosePoints(pt, ptNext, otherPoints[indexOther], otherPoints[indexOther + 1],
						startSplit, splitAt, splitPt, skipToAt, skipToPt,
						MERGE_DISTANCE, MERGE_DISTANCE2);
				if (indexOther > 0)
					SkipSplit_ClosePoints(pt, ptNext, otherPoints[indexOther], otherPoints[indexOther - 1],
						startSplit, splitAt, splitPt, skipToAt, skipToPt,
						MERGE_DISTANCE, MERGE_DISTANCE2);
			}
			if (skipToAt >= 1.f)
				break;
		}

		// After looping over the other lines and examining close
		// segments, check out the results.
		if (startSplit && start < i) {
			std::vector<VE::Point> choppedPoints(points.begin() + start, points.begin() + i + 1);
			result.push_back(std::make_shared<VE::Polyline>(choppedPoints));
			start = i;
		}
		if (skipToAt >= 1.f) { // Just go to the next point.
			start = i + 1;
			continue;
		}
		if (skipToAt > 0.f) { // Replace with next, then resume with the same point.
			points[i] = skipToPt;
			start = i;
			i--;
			continue;
		}

		if (splitAt < 1.f) {
			if (startSplit) {
				// Add short segment, replace, then resume with the replaced (same) point.
				std::vector<VE::Point> choppedPoints(points.begin() + i, points.begin() + i + 2);
				choppedPoints.back() = splitPt;
				result.push_back(std::make_shared<VE::Polyline>(choppedPoints));
				points[i] = splitPt;
				start = i;
				i--;
			}
			else {
				// Chop off with additional point, replace, then resume with the replaced (same) point.
				std::vector<VE::Point> choppedPoints(points.begin() + start, points.begin() + i + 2);
				choppedPoints.back() = splitPt;
				result.push_back(std::make_shared<VE::Polyline>(choppedPoints));
				points[i] = splitPt;
				start = i;
				i--;
			}
		}

	}
	if (start < points.size() - 1) {
		std::vector<VE::Point> choppedPoints(points.begin() + start, points.end());
		result.push_back(std::make_shared<VE::Polyline>(choppedPoints));
	}
}

// Remove floats (Curves Segments which overlap)
// and split curves into separate segments at intersections
void VectorGraphic::RemoveOverlaps()
{
	int numberOfPolylines = Polylines.size();

	// The last polyline will not overlap, as all other have been corrected.
	for (int i = 0; i < numberOfPolylines; i++)
	{
		// Take first out.
		VE::PolylinePtr polyline = Polylines.front();
		std::vector<VE::Point> points = polyline->getPoints();
		Polylines.erase(Polylines.begin());

		std::vector<VE::PolylinePtr> newPolylines;
		std::vector<VE::PolylinePtr> others;
		std::vector<VE::Bounds> paddedBounds;
		std::vector<float> distances2;
		LinesWithPaddedBounds(polyline, Polylines,
			others, paddedBounds, distances2);
		GetNoneOverlappingLines(points, newPolylines,
			others, paddedBounds, distances2);
		
		Polylines.insert(Polylines.end(), newPolylines.begin(), newPolylines.end());
		std::cout << "|";
		
	}
	std::cout << "\n";
	for (int i = Polylines.size() - 1; i >= 0; i--)
	{
		const PolylinePtr pl = Polylines[i];

		if (pl->Length() < 2) {
			std::cout << "FATAL " << i << "\n";
			Polylines.erase(Polylines.begin() + i);
			//throw std::exception("WTF");
		}
	}
			
	return;
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
	for (VE::PolylinePtr& pl : Polylines) {
		pl->ResetStatus();
	}
	int amount = -1;
	int newAmount = 0;
	while (amount != newAmount) {
		amount = newAmount;
		newAmount = 0;
		for (VE::PolylinePtr& pl : Polylines)
		{
			auto otherLines = Polylines;
			otherLines.erase(std::find(otherLines.begin(), otherLines.end(), pl));

			// compute front
			pl->ConnectFront = GetConnections(pl->Front(), otherLines);
			pl->ConnectBack = GetConnections(pl->Back(), otherLines);
			pl->UpdateStatus();
			if (pl->Status() == Polyline::LineStat::invalid) {
				newAmount++;
			}
		}
	}
}

void VectorGraphic::RemoveUnusedConnections()
{
	decltype(Polylines) validLines;
	for (VE::PolylinePtr& pl : Polylines)
	{
		if (pl->Status() != Polyline::LineStat::invalid)
			validLines.push_back(pl);
	}
	Polylines = std::move(validLines);
}

void VectorGraphic::Split(VE::PolylinePtr pl, const int& idx)
{
	std::vector<int> indices{ idx };
	Split(pl, indices);
}

void VectorGraphic::Split(VE::PolylinePtr pl, const int& idx, const float& t)
{
	const std::vector<VE::Point>& orig = pl->getPoints();
	assert(idx >= 0 || idx < orig.size());
	assert(std::abs(t) > 0 && std::abs(t) < 1);

	Polylines.erase(std::find(Polylines.begin(), Polylines.end(), pl));
	DeleteConnections(pl);

	float t1 = t;
	int idx1 = idx;
	if (t < 0) {
		t1 = 1 + t;
		idx1 = idx - 1;
		
	}

	VE::Point uv = orig[idx1 + 1] - orig[idx1];
	std::vector<VE::Point> splitPoints1(orig.begin(), orig.begin() + idx1 + 2);
	std::vector<VE::Point> splitPoints2(orig.begin() + idx1, orig.end());
	splitPoints1.back() = orig[idx1] + uv * t1;
	splitPoints2.front() = splitPoints1.back();
	Polylines.push_back(std::make_shared<VE::Polyline>(splitPoints1));
	Polylines.push_back(std::make_shared<VE::Polyline>(splitPoints2));
}

void VectorGraphic::Split(VE::PolylinePtr pl, const std::vector<int> indices)
{
	auto& orig = pl->getPoints();

	for (const int& idx : indices) {
		if (idx < 0 || idx >= orig.size())
			throw std::invalid_argument("Point not on polyline.");
	}

	std::vector<int> sIndices(indices);
	sort(sIndices.begin(), sIndices.end());
	
	Polylines.erase(std::find(Polylines.begin(), Polylines.end(), pl));
	DeleteConnections(pl);

	int last = 0;
	for (int i = 0; i < sIndices.size(); i++)
	{
		if (sIndices[i] <= last)
			continue;

		std::vector<VE::Point> splitPoints(orig.begin() + last, orig.begin() + sIndices[i] + 1);
		Polylines.push_back(std::make_shared<VE::Polyline>(splitPoints));

		last = sIndices[i];
	}
	if (last < orig.size() - 1) {
		std::vector<VE::Point> splitPoints(orig.begin() + last, orig.end());
		Polylines.push_back(std::make_shared<VE::Polyline>(splitPoints));
	}
}

/*
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
*/

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

void VectorGraphic::MakeColorsUnique()
{
	for (auto psA = Polyshapes.begin(); psA != Polyshapes.end(); psA++)
	{
		ColorAreaPtr col = (*psA)->getColor();
		for (auto psB = psA + 1; psB != Polyshapes.end(); psB++) {
			if ((*psB)->getColor() == col) {
				(*psB)->setColor(col);
			}
		}
	}
}


VectorGraphic::CPParams::CPParams(const VE::Point& target, M method)
	: target(target), method(method)
{
}

// Try snapping endpoints if requested.
void TrySnap(VectorGraphic::CPParams& params) {
	float startDist2 = VE::Distance2(params.closestPolyline->Front(), params.closestPt);
	float endDist2 = VE::Distance2(params.closestPolyline->Back(), params.closestPt);
	if (startDist2 < params.snapEndpoints2 || endDist2 < params.snapEndpoints2) {
		if (startDist2 < endDist2) {
			params.ptIdx = 0;
			params.closestPt = params.closestPolyline->Front();
		}
		else {
			params.ptIdx = params.closestPolyline->Length() - 1;
			params.closestPt = params.closestPolyline->Back();
		}
	}
}


void VectorGraphic::ClosestPolyline(CPParams& params)
{
	params.ptIdx = -1;
	params.closestPolyline = nullptr;

	// Closest Line and Point Indices are kept for debugging.
	// Perhaps this function should simply return indices instead of references.
	for (auto& pl : Polylines) {
		// Check for visibility first.
		if (params.bounds != nullptr &&
			!params.bounds->Overlap(pl->getBounds()))
			continue;
		
		int index;
		if (params.method == CPParams::M::Point)
			index = pl->ClosestPt2(params.target, params.distance2);
		else
			index = pl->ClosestLinePt2(params.target, params.distance2, params.closestPt, params.t);

		if (index >= 0) {
			params.ptIdx = index;
			params.closestPolyline = pl;
		}
	}
	if (params.ptIdx == -1)
		return;

	// With splitting, we don't need to make the closest point an element on the line.
	if (params.method != CPParams::M::Split)
		params.closestPt = params.closestPolyline->getPoints()[params.ptIdx];

	if (params.snapEndpoints2 > 0)
		TrySnap(params);

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
	if (Polylines.empty())
		return Bounds(0, 0, 100, 100);

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
