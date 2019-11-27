#include "Connection.h"

#include "Polyline.h"

namespace VE {

	void Connection::Invert() {
		if (at == Connection::start)
			at = Connection::end;
		else at = Connection::start;
	}

	Point& Connection::StartPoint() {
		if (at == Connection::start)
			return polyline->Front();
		return polyline->Back();
	}

	Point& Connection::EndPoint() {
		if (at == Connection::start)
			return polyline->Back();
		return polyline->Front();
	}

	Point& Connection::StartPoint1()
	{
		if (at == Connection::start)
			return polyline->Front1();
		return polyline->Back1();
	}

	Point& Connection::EndPoint1()
	{
		if (at == Connection::start)
			return polyline->Back1();
		return polyline->Front1();
	}

	void Connection::AppendTo(std::vector<Point>& points, const int& removeLast = 0)
	{
		auto& polypoints = polyline->getPoints();

		if (at == Connection::start)
			points.insert(points.end(), polypoints.begin(), polypoints.end() - removeLast);
		else
			points.insert(points.end(), polypoints.rbegin(), polypoints.rend() - removeLast);
	}

	inline bool is_left(const Point& a, const Point& b, const Point& pt) {
		return 0 >= (b.x - a.x) * (pt.y - a.y) - (b.y - a.y) * (pt.x - a.x);
	}

	void Connection::SortOther(std::vector<Connection>& others)
	{
		std::vector<Connection> sorted(others.size());
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
		}
		others = std::move(sorted);
	}
}