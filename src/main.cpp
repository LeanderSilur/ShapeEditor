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
	if (argc > 1 && std::string(argv[1]) == "debug") {
		std::cout << "In debugging.";
		//std::string filename = "D:/download/malila3_09/OUT_A/0076.png";
		std::string filename = "D:/download/malila_coloring_character/7_05/malila/0051.png";
		Animation::Frame frame(filename, filename + ".l.svg");

		iw->AddFrame(frame);
		iw->NextFrame(true);
	}

	se.show();
	return a.exec();
}