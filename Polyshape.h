#pragma once

#include "VectorElement.h"

namespace VE {

	class Polyshape : public VectorElement {
	protected:
		bool counterClockwise = true;

		std::vector<Connection> connections;
		bool valid = false;
		std::vector<Point> points;
		VE::ColorAreaPtr color = std::make_shared<ColorArea>();


	public:
		void Cleanup();

		Polyshape();

		std::vector<Point>& getPoints();
		VE::ColorAreaPtr getColor();
		void setColor(VE::ColorAreaPtr&col);

		void setConnections(std::vector<Connection>& connections);
		const std::vector<Connection>& getConnections();

		bool Valid() { return valid; };

		void Draw(cv::Mat& img, Transform& t);
		bool AnyPointInRect(Bounds& rect);

	private:
		void CalculateDirection();
	public:
		bool CounterClockwise();
	};

}