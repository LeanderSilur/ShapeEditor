#pragma once

#include <vector>
#include <iostream>
#include <opencv2/core.hpp>
#include "VectorGraphic.h"

namespace Animation {
	class Frame {
		cv::Mat image = cv::Mat::zeros(16, 16, CV_8UC3);
		VectorGraphic vectorGraphic;
		std::string name = "N/A";
	public:
		Frame();
		Frame(const cv::Mat& image, const VectorGraphic& graphic);
		Frame(std::string image_path, std::string line_path);

		// Loads the svg from the given path. Tries to find
		// a matching .png from: "myfileName0000.png.svg"
		bool Load(std::string image_path);
		bool Load(std::string image_path, std::string line_path);

		cv::Mat& getImage();
		VectorGraphic& getVectorGraphic();
		const std::string& getName();
	};
}