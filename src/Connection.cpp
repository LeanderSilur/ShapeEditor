#include "Connection.h"
#include "Polyline.h"
#include <iostream>

namespace VE {
	Connection::Connection()
	{
		polyline = nullptr;
	}

	Connection::Connection(PolylinePtr& pl, Location l)
	{
		polyline = pl;
		at = l;
	}

	void Connection::Invert() {
		if (at == Location::start)
			at = Location::end;
		else at = Location::start;
	}

	const Point& Connection::StartPoint() {
		if (at == Location::start)
			return polyline->Front();
		return polyline->Back();
	}

	const Point& Connection::EndPoint() {
		if (at == Location::start)
			return polyline->Back();
		return polyline->Front();
	}

	const Point& Connection::StartPoint1()
	{
		if (at == Location::start)
			return polyline->Front1();
		return polyline->Back1();
	}

	const Point& Connection::EndPoint1()
	{
		if (at == Location::start)
			return polyline->Back1();
		return polyline->Front1();
	}

	void Connection::AppendTo(std::vector<Point>& points, const int& removeLast = 0)
	{
		auto& polypoints = polyline->getPoints();

		if (at == Location::start)
			points.insert(points.end(), polypoints.begin(), polypoints.end() - removeLast);
		else
			points.insert(points.end(), polypoints.rbegin(), polypoints.rend() - removeLast);
	}

	inline bool is_left(const Point& a, const Point& b, const Point& pt) {
		return 0 >= (b.x - a.x) * (pt.y - a.y) - (b.y - a.y) * (pt.x - a.x);
	}

	void Connection::SortOther(std::vector<Connection>& others)
	{
		std::vector<Connection> sorted;
		VE::Point a = EndPoint1(),
			b = EndPoint();

		while (others.size() > 0) {
			auto leftmostCon = others.begin();
			bool on_left = is_left(a, b, leftmostCon->StartPoint1());

			for (size_t i = 1; i < others.size(); i++)
			{
				auto nextCon = others.begin() + i;

				bool next_on_left = is_left(a, b, nextCon->StartPoint1());
				bool con_on_left = is_left(b, leftmostCon->StartPoint1(), nextCon->StartPoint1());
				if ((on_left && next_on_left && con_on_left) ||
					(!on_left && next_on_left) ||
					(!on_left && con_on_left))
				{
					leftmostCon = nextCon;
					on_left = next_on_left;
				}
			}
			sorted.push_back(*leftmostCon);
			others.erase(leftmostCon);
		}
		others = std::move(sorted);
	}

	float Connection::AngleArea()
	{
		float angle = 0;
		const std::vector<VE::Point>& points = polyline->getPoints();

		for (auto it = points.begin() + 1; it != points.end(); it++)
		{
			angle += ((*it).x - (*(it - 1)).x) * ((*it).y + (*(it - 1)).y);
		}
		if (at == Location::end)
			angle = -angle;

		return angle;
	}
}