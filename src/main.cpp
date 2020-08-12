#include <iostream>
#include <QtWidgets/QApplication>
#include <QtWidgets>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include "ImageViewer.h"
#include "ShapeEditor.h"
#include "InputDialog.h"

#include <filesystem>
#include "PtTree.h"

#include "AnimationFrame.h"

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);

	ShapeEditor se;
	ImageViewer *iw = new ImageViewer(se.getUi().viewerGrp);
	iw->ConnectUi(se);
	if (std::string(argv[1]) == "debug") {
		std::cout << "In debugging.";

		std::string filename = "D:/190725_sequence_colorization/files/_arc/w.png";
		Animation::Frame frame(filename, filename + ".svg");

		iw->AddFrame(frame);
		iw->NextFrame(true);
		//iw->RemoveOverlaps(true);
	}
	se.show();



	

	return a.exec();
}