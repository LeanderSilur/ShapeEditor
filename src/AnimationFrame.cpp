#include "AnimationFrame.h"

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <filesystem>

Animation::Frame::Frame()
{
}

Animation::Frame::Frame(const cv::Mat& image, const VectorGraphic& graphic)
{
	this->image = image;
	this->vectorGraphic = graphic;
}

Animation::Frame::Frame(std::string image_path, std::string line_path)
{
	Load(image_path, line_path);
}

bool VerifyPath(std::string& path) {
	if (!std::filesystem::is_regular_file(path)) {
		std::cout << "File not found: " << path << "\n";
		return false;
	}
	return true;
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

bool Animation::Frame::Load(std::string line_path)
{
	std::string image_path(line_path);
	replace(image_path, ".l.svg", "");
	replace(image_path, ".svg", "");
	return Load(image_path, line_path);
}

bool Animation::Frame::Load(std::string image_path, std::string line_path)
{
	if (VerifyPath(image_path)) {
		image = cv::imread(image_path, cv::IMREAD_UNCHANGED);
		if (image.empty()) {
			return false;
		}
		if (image.channels() == 4) {
			std::vector<cv::Mat> imageChannels;
			cv::split(image, imageChannels);

			//invert
			/*auto ptr = imageChannels[3].ptr();
			for (uint i = 0; i < imageChannels[0].cols * imageChannels[0].rows; i++)
			{
				ptr[i] = 255 - ptr[i];
			} */

			imageChannels[1] = imageChannels[3];
			imageChannels[2] = imageChannels[3];
			imageChannels.erase(imageChannels.begin());

			cv::merge(imageChannels, image);
			
		}
		else {
			cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
		}
	}
	else {
		return false;
	}

	if (VerifyPath(line_path)) {
		vectorGraphic.Load(line_path);
	}
	else {
		return false;
	}

	name = std::filesystem::path(image_path).filename().string();
	replace(line_path, ".l.svg", ".svg");
	linePath = line_path;
	return true;
}

bool Animation::Frame::ReloadOrig()
{
	if (VerifyPath(linePath))
		return Load(linePath);
	return false;
}

bool Animation::Frame::ReloadEdit()
{
	std::string editLinePath = linePath;
	replace(editLinePath, ".svg", ".l.svg");
	if (VerifyPath(editLinePath))
		return Load(editLinePath);
	return false;
}

cv::Mat& Animation::Frame::getImage()
{
	return image;
}

VectorGraphic& Animation::Frame::getVectorGraphic()
{
	return vectorGraphic;
}

const std::string& Animation::Frame::getName()
{
	return name;
}

std::string Animation::Frame::getEditName()
{
	return name + ".l.svg";
}

std::string Animation::Frame::getShapeName()
{
	return name + ".s.svg";
}
