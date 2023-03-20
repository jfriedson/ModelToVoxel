// Convert a traditional 3D model and its texture into an octree with color, transparency, and triangle-face-derived normal data.
// Optionally, view a previously converted model.

#include "App.h"


int main() {
	App::AppProperties props;
	props.convertMesh = true;
	props.svoPath = "gallerygpu.svo";

	// required if convertMesh is true
	props.conversionDevice = "GPU"; // property not yet used, CPU octree building still being reimplemented
	props.modelPath = "./assets/gallery.obj";
	props.texPath = "./assets/gallery.jpg";
	props.octreeDepth = 11;
	props.saveModel = true;


	App app(props);
	app.run();

	return 0;
}
