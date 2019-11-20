#include <iostream>
#include <QtWidgets/QApplication>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include "ImageViewer.h"



int main(int argc, char* argv[])
{


	cv::Mat im = cv::imread("D:/190725_sequence_colorization/arc/test_files/w.png");
	VectorGraphic vg;
	vg.LoadPolylines("D:/190725_sequence_colorization/PolyV_qt/x64/Release/w.pngLala.svg");

	QApplication a(argc, argv);
	ImageViewer w;

	w.setGraphic(vg);
	w.setMat(im);

	w.show();
	return a.exec();
}
