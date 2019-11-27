#pragma once

#include "Constants.h"

namespace VE {
	class Connection {
	public:
		enum Location { start, end };
		PolylinePtr polyline;
		Location at;

		void Invert();
		Point& StartPoint();
		Point& EndPoint();
		Point& StartPoint1();
		Point& EndPoint1();
		void AppendTo(std::vector<Point>& points, const int & removeLast);
		void SortOther(std::vector<Connection>& others);
	};
}