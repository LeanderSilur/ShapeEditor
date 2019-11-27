#pragma once

#include "VectorElement.h"

namespace VE {

	class Polyshape : public VectorElement {
	protected:
		std::vector<Connection> connections;
		Bounds bounds;
		bool valid;
		std::vector<Point> points;


	public:
		void Cleanup();

		Polyshape();

		void setConnections(std::vector<Connection>& connections);
		std::vector<Connection>& getConnections();

		bool Valid() { return valid; };

		void Draw(cv::Mat& img, Transform& t, bool highlight = false);
		bool AnyPointInRect(Bounds& rect);

		Bounds& getBounds();

	};

}