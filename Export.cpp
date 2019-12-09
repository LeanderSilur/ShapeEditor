#include "Export.h"
#include "Polyline.h"
#include "Polyshape.h"
#include <fstream>

void ExportPoints(std::string path, std::string image_path, cv::Size2i shape, std::vector<std::vector<VE::Point>*>& lines, bool fill)
{
	std::ofstream savefile;
	savefile.open(path, std::ios::out);
	savefile << "<svg xmlns='http://www.w3.org/2000/svg' width='"
		<< shape.width << "' height='" << shape.height << "'	 >"
		<< R"(
	<style type = 'text/css'>
		<![CDATA[
			image{
				image - rendering: -moz - crisp - edges;
				image - rendering:   -o - crisp - edges;
				image - rendering: pixelated;
			}
			polyline{
					stroke - width:0.5px;
					stroke:#f00;
)";
	if (!fill) savefile << "		fill:none;\n";
	savefile << R"(			}
		]]>
	</style>)";

	savefile << "<image href='" << image_path << "' />\n";

	for (auto& points : lines)
	{
		savefile << "<polyline points='";
		for (auto& pt : *points) {
			savefile << pt.x << "," << pt.y << " ";
		}
		savefile << "' />\n";
	}
	savefile << "</svg>";
	savefile.close();
}
void Export::SaveSVG(std::string path, std::string image_path, cv::Size2i shape, std::vector<VE::PolylinePtr>& polylines)
{
	std::vector<std::vector<VE::Point>*> lines;
	for (auto&pl:polylines)
		lines.push_back(&pl->getPoints());

	ExportPoints(path, image_path, shape, lines, false);
}


void Export::SaveSVG(std::string path, std::string image_path, cv::Size2i shape, std::vector<VE::PolyshapePtr>& polyshapes)
{
	std::ofstream savefile;
	savefile.open(path, std::ios::out);
	savefile << "<svg xmlns='http://www.w3.org/2000/svg' width='"
		<< shape.width << "' height='" << shape.height << "'	 >"
		<< R"(
	<style type = 'text/css'>
		<![CDATA[
			image{
				image - rendering: -moz - crisp - edges;
				image - rendering:   -o - crisp - edges;
				image - rendering: pixelated;
			}
			polyline{
					stroke - width:0.5px;
					stroke:#000;
			}
		]]>
	</style>)";

	savefile << "<image href='" << image_path << "' />\n";

	for (auto& ps : polyshapes)
	{
		auto& points = ps->getPoints();

		savefile << "<polyline points='";
		for (auto& pt : points) {
			savefile << pt.x << "," << pt.y << " ";
		}
		auto& col = ps->getColor();
		savefile << "' fill=\"rgb("
			<< col->Color[0] << ", " 
			<< col->Color[1] << ", " 
			<< col->Color[2] << ")\" name=\""
			<< col->Name << "\" direction=\""
			<< ps->CounterClockwise() << "\" />\n";
	}
	savefile << "</svg>";
	savefile.close();
}
