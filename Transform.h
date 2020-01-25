#pragma once

#include "Constants.h"
#include "Bounds.h"

namespace VE {
	class Transform
	{
	public:
		float scale = 1.0f;
		float x, y;

		void apply(Point& pt);
		void applyInv(Point& pt);
		void apply(Bounds& bounds);
		void applyInv(Bounds& bounds);


		void apply(float& d);
		void applyInv(float& d);
		void Reset();
	};
}
