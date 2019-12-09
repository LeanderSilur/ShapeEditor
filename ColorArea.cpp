#include "ColorArea.h"

VE::ColorArea::ColorArea()
{
}

VE::ColorArea::ColorArea(std::string name, cv::Scalar color)
{
	Name = name;
	Color = color;
}

VE::ColorArea::ColorArea(std::string name, double r, double g, double b)
{
	Name = name;
	Color = cv::Scalar(r, g, b);
}
