#pragma once

#include "Constants.h"

namespace VE {
	class Connection {
	public:
		enum class Location { start, end };
		Connection();
		Connection(PolylinePtr& pl, Location l);
		
		inline bool operator==(const Connection& other) {
			if (other.polyline != polyline) return false;
			if (other.at != at) return false;
			return true;
		};

		PolylinePtr polyline;
		Location at;

		void Invert();
		Point& StartPoint();
		Point& EndPoint();
		Point& StartPoint1();
		Point& EndPoint1();
		void AppendTo(std::vector<Point>& points, const int & removeLast);
		void SortOther(std::vector<Connection>& others);
		float AngleArea(Connection & other);
	};
}