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
	if (argc > 1) {
		std::string example_path = std::string(argv[1]);
		std::cout << example_path;
		if (!std::filesystem::is_regular_file(example_path)) {
			std::cout << "Example input file not found.\n";
		}
		else {
			std::string filename = example_path;
			Animation::Frame frame(filename, filename + ".l.svg");
			iw->AddFrame(frame);
			iw->NextFrame(true);
		}
	}

	se.show();
	return a.exec();
}