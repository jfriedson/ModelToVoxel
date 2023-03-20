// Convert a traditional 3D model and its texture into an octree with color, transparency, and triangle-face-derived normal data.
// Optionally, view a previously converted model.

#include "app.h"


int main() {
	App::AppProperties props;
	props.convertMesh = true;
	props.svoPath = "asdf.svo";

	// required if convertMesh is true
	props.conversionDevice = "GPU"; // CPU octree building not yet converted
	props.modelPath = "./assets/mirage.obj";
	props.texPath = "./assets/mirage.png";
	props.octreeDepth = 5;
	props.saveModel = true;


	App app(props);
	app.run();

	return 0;
}
