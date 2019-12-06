#pragma once

#include "Constants.h"

class Export {
public:
	static void SaveSVG(std::string path, std::string image_path, cv::Size2i shape, std::vector<VE::PolylinePtr>& polylines);
	static void SaveSVG(std::string path, std::string image_path, cv::Size2i shape, std::vector<VE::PolyshapePtr>& polyshapes);
};


