#pragma once

#include <opencv2/core.hpp>
#include <string>

class ColorArea {
public:
	ColorArea();
	ColorArea(std::string name, cv::Scalar color);
	ColorArea(std::string name, double r, double g, double b);

	inline bool operator==(const ColorArea& other) {
		if (other.Name != Name) return false;
		if (other.Color != Color) return false;
		return true;
	};

	std::string Name = "N/A";
	cv::Scalar Color = cv::Scalar(255, 0, 0);
};