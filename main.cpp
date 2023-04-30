// Convert a traditional 3D model and its texture into an octree with color, transparency, and triangle-face-derived normal data.
// Optionally, view a previously converted model.

#include "App.h"


int main() {
	App& app = App::getInstance();

	app.props.convertMesh = true;
	app.props.svoPath = "gallerygpu.svo";

	// required if convertMesh is true
	app.props.conversionDevice = "GPU"; // property not yet used, CPU octree building still being reimplemented
	app.props.modelPath = "./assets/gallery.obj";
	app.props.texPath = "./assets/gallery.jpg";
	app.props.octreeDepth = 11;
	app.props.saveModel = true;

	app.run();

	return 0;
}
