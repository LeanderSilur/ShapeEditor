#include "Transform.h"

namespace VE {
	void Transform::apply(Point& pt)
	{
		pt *= scale;
		pt.x += x; pt.y += y;
	}
	void Transform::applyInv(Point& pt)
	{
		pt.x -= x; pt.y -= y;
		pt /= scale;
	}

	void Transform::apply(Bounds& bounds)
	{
		bounds.x0 *= scale; bounds.y0 *= scale;
		bounds.x1 *= scale; bounds.y1 *= scale;
		bounds.x0 += x; bounds.y0 += y;
		bounds.x1 += x; bounds.y1 += y;
	}
	void Transform::applyInv(Bounds& bounds)
	{
		bounds.x0 -= x; bounds.y0 -= y;
		bounds.x1 -= x; bounds.y1 -= y;
		bounds.x0 /= scale; bounds.y0 /= scale;
		bounds.x1 /= scale; bounds.y1 /= scale;
	}

	void Transform::apply(float& d)
	{
		d *= scale;
	}
	void Transform::applyInv(float& d)
	{
		d /= scale;
	}
	void Transform::Reset()
	{
		x = 0;
		y = 0;
		scale = 1.0f;
	}
}
