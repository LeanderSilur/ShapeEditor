#pragma once

#include "VectorElement.h"

namespace VE {

	class Polyshape : public VectorElement {
	protected:
		std::vector<Connection> connections;
		bool valid;
		std::vector<Point> points;


	public:
		void Cleanup();

		Polyshape();

		std::vector<Point>& getPoints();

		void setConnections(std::vector<Connection>& connections);
		std::vector<Connection>& getConnections();

		bool Valid() { return valid; };

		void Draw(cv::Mat& img, Transform& t, const cv::Scalar &color);
		bool AnyPointInRect(Bounds& rect);

	};

}