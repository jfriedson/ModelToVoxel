#include "App.h"

#include <iostream>
#include <numeric>
#include <random>
#include <thread>
#include <vector>




App::App(AppProperties& props) : props( props ) {
	createWindow();

	setupShaders();

	setupObjects();
}

App::~App() {
	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteTextures(1, &renderShaderProperties.rtTexture);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &renderShaderProperties.quadVAO);

	glDeleteBuffers(1, &sharedProperties.inner_octant_buf);
	glDeleteBuffers(1, &sharedProperties.leaf_buffer);

	glDeleteProgram(renderShaderProperties.quadProgram);
	glDeleteProgram(renderShaderProperties.rtProgram);
}


void App::createWindow() {
	windowProperties.window = voxgl::createWindow("Voxel converter and ray tracer", windowProperties.windowWidth, windowProperties.windowHeight, false);
}


void App::setupShaders() {
	setupRenderShader();
}

void App::setupRenderShader() {
	glfwGetFramebufferSize(windowProperties.window.get(), &renderProperties.framebufferWidth, &renderProperties.framebufferHeight);

	// increase performance for high resolution monitors if needed
	/*
	if (windowProperties.windowWidth > 1920 || windowProperties.windowHeight > 1080) {
		renderProperties.framebufferWidth /= 2;
		renderProperties.framebufferHeight /= 2;
	}
	*/

	// workgroup size for rendering
	renderShaderProperties.rtX = renderProperties.framebufferWidth / renderShaderProperties.rtWork + (renderProperties.framebufferWidth % renderShaderProperties.rtWork != 0);
	renderShaderProperties.rtY = renderProperties.framebufferHeight / renderShaderProperties.rtWork + (renderProperties.framebufferHeight % renderShaderProperties.rtWork != 0);

	// quad to display texture
	glGenVertexArrays(1, &renderShaderProperties.quadVAO);

	// ray trace texture
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &renderShaderProperties.rtTexture);
	glBindTexture(GL_TEXTURE_2D, renderShaderProperties.rtTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, renderProperties.framebufferWidth, renderProperties.framebufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glBindImageTexture(0, renderShaderProperties.rtTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

	// load quad shaders
	GLuint quadVertShader = voxgl::createShader("./shaders/quad.vs", GL_VERTEX_SHADER);
	GLuint quadFragShader = voxgl::createShader("./shaders/quad.fs", GL_FRAGMENT_SHADER);
	std::vector<GLuint> quadShaders;
	quadShaders.emplace_back(quadVertShader);
	quadShaders.emplace_back(quadFragShader);
	renderShaderProperties.quadProgram = voxgl::createProgram(quadShaders);

	loadRTShader();
}


void App::setupObjects() {
	worldObjects.player.position = glm::vec3{-.3f, .7f, -.3f};
	worldObjects.player.camera.direction = glm::vec2{ .8f, 0.f };
}


void App::loadRTShader() {
	GLuint rtCompShader = voxgl::createShader("./shaders/raytrace.comp", GL_COMPUTE_SHADER);
	std::vector<GLuint> rtShaders;
	rtShaders.emplace_back(rtCompShader);

	GLint shaderRef = voxgl::createProgram(rtShaders);
	if (shaderRef != -1) {
		renderShaderProperties.rtProgram = shaderRef;

		renderShaderProperties.resolutionUniform = glGetUniformLocation(renderShaderProperties.rtProgram, "resolution");
		renderShaderProperties.camPosUniform = glGetUniformLocation(renderShaderProperties.rtProgram, "cameraPos");
		renderShaderProperties.camMatUniform = glGetUniformLocation(renderShaderProperties.rtProgram, "cameraMat");
		renderShaderProperties.sunDirUniform = glGetUniformLocation(renderShaderProperties.rtProgram, "sunDir");

		glUseProgram(renderShaderProperties.rtProgram);
		glUniform2f(renderShaderProperties.resolutionUniform, static_cast<float>(renderProperties.framebufferWidth), static_cast<float>(renderProperties.framebufferHeight));
		glUniform3f(renderShaderProperties.sunDirUniform, 0.07f, .9f, 0.03f);
	}
}


void App::loadSVO() {
	if (props.convertMesh)
		convertMeshToVoxels();
	else
		worldObjects.octree = std::make_unique<SVO<InnerOctant, Leaf, MaskType>>(props.svoPath);

	// fitted octree buffers
	glGenBuffers(1, &renderShaderProperties.inner_octant_buf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, renderShaderProperties.inner_octant_buf);
	glBufferData(GL_SHADER_STORAGE_BUFFER, worldObjects.octree->getInnerOctantsSize(), worldObjects.octree->getInnerOctantData(), GL_STATIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, renderShaderProperties.inner_octant_buf);

	glGenBuffers(1, &renderShaderProperties.leaf_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, renderShaderProperties.leaf_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, worldObjects.octree->getLeavesSize(), worldObjects.octree->getLeafData(), GL_STATIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, renderShaderProperties.leaf_buffer);

	// bind quad program VAO
	glBindVertexArray(renderShaderProperties.quadVAO);
}

void App::convertMeshToVoxels() {
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glfwSwapBuffers(windowProperties.window.get());

	worldObjects.octree = std::make_unique<SVO<InnerOctant, Leaf, MaskType>>(props.octreeDepth);

	const unsigned int dimension = glm::pow(2, props.octreeDepth);

	GLuint modelToVoxelsVertShader = voxgl::createShader("./shaders/convertMeshToVoxels.vs", GL_VERTEX_SHADER);
	GLuint modelToVoxelsGeomShader = voxgl::createShader("./shaders/convertOctree.gs", GL_GEOMETRY_SHADER);
	GLuint modelToVoxelsFragShader = voxgl::createShader("./shaders/convertMeshToVoxels.fs", GL_FRAGMENT_SHADER);
	std::vector<GLuint> modelToVoxelsShaders;
	modelToVoxelsShaders.emplace_back(modelToVoxelsVertShader);
	modelToVoxelsShaders.emplace_back(modelToVoxelsGeomShader);
	modelToVoxelsShaders.emplace_back(modelToVoxelsFragShader);
	GLuint modelToVoxelsProgram = voxgl::createProgram(modelToVoxelsShaders);

	GLuint treeDepthUniform = glGetUniformLocation(modelToVoxelsProgram, "treeDepth");
	GLuint voxelResolutionUniform = glGetUniformLocation(modelToVoxelsProgram, "voxelResolution");

	glUseProgram(modelToVoxelsProgram);
	glUniform1ui(treeDepthUniform, props.octreeDepth);
	glUniform1f(voxelResolutionUniform, static_cast<float>(dimension));

	Model model(props.modelPath);

	Texture texture;
	texture.LoadTextureLinear(props.texPath, 1);

	// atomic counters
	GLuint countersInitial[2] = { 1, 1 };
	GLuint nodeIndicesCounter;
	glGenBuffers(1, &nodeIndicesCounter);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, nodeIndicesCounter);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint) * 2, &countersInitial, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, nodeIndicesCounter);

	// inner node buffer
	worldObjects.octree->setInnerOctantsSize((0x80000000u / sizeof(InnerOctant)) - 1);
	GLuint nodeBuffer;
	glGenBuffers(1, &nodeBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodeBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 0x80000000u - sizeof(InnerOctant), worldObjects.octree->getInnerOctantData(), GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, nodeBuffer);
	worldObjects.octree->setInnerOctantsSize(1);

	// leaf buffer
	worldObjects.octree->setLeavesSize((0x80000000u / sizeof(Leaf)) - 1);
	GLuint leafBuffer;
	glGenBuffers(1, &leafBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, leafBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 0x80000000u - sizeof(Leaf), worldObjects.octree->getLeafData(), GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, leafBuffer);
	worldObjects.octree->setLeavesSize(1);

	// timing query object
	GLuint timerQuery;
	glGenQueries(1, &timerQuery);

	glBeginQuery(GL_TIME_ELAPSED, timerQuery);

	// conversion
	model.render();
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	glEndQuery(GL_TIME_ELAPSED);
	GLint queryAvailable = 0;
	GLuint elapsed_time;

	while (!queryAvailable) {
		glGetQueryObjectiv(timerQuery,
			GL_QUERY_RESULT_AVAILABLE,
			&queryAvailable);
	}
	glGetQueryObjectuiv(timerQuery, GL_QUERY_RESULT, &elapsed_time);
	glDeleteQueries(1, &timerQuery);


	std::cout << "model conversion: " << elapsed_time / 1000 << "us\n";

	texture.~Texture();
	model.~Model();

	// get voxel data from gpu
	GLuint voxelCount[2];
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, nodeIndicesCounter);
	glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint) * 2, &voxelCount);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	glDeleteBuffers(1, &nodeIndicesCounter);

	std::cout << voxelCount[0] << " elements in octree\n";
	std::cout << voxelCount[1] << " leaves\n";

	worldObjects.octree->setInnerOctantsSize(voxelCount[0]);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodeBuffer);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(InnerOctant) * voxelCount[0], reinterpret_cast<void*>(const_cast<InnerOctant* const>(worldObjects.octree->getInnerOctantData())));
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glDeleteBuffers(1, &nodeBuffer);

	worldObjects.octree->setLeavesSize(voxelCount[1]);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, leafBuffer);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 1, sizeof(Leaf) * voxelCount[1], reinterpret_cast<void*>(const_cast<Leaf* const>(worldObjects.octree->getLeafData())));
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glDeleteBuffers(1, &leafBuffer);

	glDeleteProgram(modelToVoxelsProgram);

	if (props.saveModel)
		worldObjects.octree->save(props.svoPath);

	worldObjects.octree->printTree(false);
}

void App::run() {
	loadSVO();

	// mutex for shader uniform variables
	std::mutex dataLock;

	std::thread render_thread = renderThread(dataLock);
	inputHandler(dataLock);

	render_thread.join();
}

const std::thread App::renderThread(std::mutex& dataLock) {
	glfwMakeContextCurrent(NULL);
	std::thread render_thread([&]() {
		glfwMakeContextCurrent(windowProperties.window.get());

		Timer renderPacer(60);

		GLdouble last_refresh = glfwGetTime();
		//int frames = 0;

		std::vector<unsigned int> renderTimes, drawTimes;
		GLuint timerQueries[2];
		glGenQueries(2, timerQueries);

		while (!glfwWindowShouldClose(windowProperties.window.get())) {
			// ray trace compute shader
			glUseProgram(renderShaderProperties.rtProgram);

			dataLock.lock();
			glUniform3f(renderShaderProperties.camPosUniform, worldObjects.player.camera.position.x, worldObjects.player.camera.position.y, worldObjects.player.camera.position.z);
			glUniformMatrix3fv(renderShaderProperties.camMatUniform, 1, GL_FALSE, glm::value_ptr(glm::transpose(worldObjects.player.camera.getMatrix())));

			if (glfwGetKey(windowProperties.window.get(), GLFW_KEY_L)) {
				glm::vec3 sunDir = -glm::normalize(worldObjects.player.camera.facingRay());
				glUniform3f(renderShaderProperties.sunDirUniform, sunDir.x, sunDir.y, sunDir.z);
			}
			dataLock.unlock();

			glBeginQuery(GL_TIME_ELAPSED, timerQueries[0]);
			glDispatchCompute(renderShaderProperties.rtX, renderShaderProperties.rtY, 1);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			glEndQuery(GL_TIME_ELAPSED);


			// draw ray trace texture to quad
			glBeginQuery(GL_TIME_ELAPSED, timerQueries[1]);
			glClear(GL_COLOR_BUFFER_BIT);
			glUseProgram(renderShaderProperties.quadProgram);
			glDrawArrays(GL_TRIANGLES, 0, 3);
			glEndQuery(GL_TIME_ELAPSED);

			glfwSwapBuffers(windowProperties.window.get());


			// get timer queries from opengl
			GLint queryAvailable = 0;
			GLuint elapsed_time;


			queryAvailable = 0;
			while (!queryAvailable) {
				glGetQueryObjectiv(timerQueries[0],
					GL_QUERY_RESULT_AVAILABLE,
					&queryAvailable);
			}
			glGetQueryObjectuiv(timerQueries[0], GL_QUERY_RESULT, &elapsed_time);
			renderTimes.push_back(elapsed_time / 1000);

			queryAvailable = 0;
			while (!queryAvailable) {
				glGetQueryObjectiv(timerQueries[1],
					GL_QUERY_RESULT_AVAILABLE,
					&queryAvailable);
			}
			glGetQueryObjectuiv(timerQueries[1], GL_QUERY_RESULT, &elapsed_time);
			drawTimes.push_back(elapsed_time / 1000);

			//frames++;
			if (int(glfwGetTime() - last_refresh) > 0) {
				//std::cout << frames << "fps" << std::endl;
				//frames = 0;

				auto const renderCount = static_cast<float>(renderTimes.size());
				auto const drawCount = static_cast<float>(drawTimes.size());

				std::cout << "render time: " << std::reduce(renderTimes.begin(), renderTimes.end()) / renderCount << "us\t" << "draw time: " << std::reduce(drawTimes.begin(), drawTimes.end()) / drawCount << "us" << std::endl;

				renderTimes.clear();
				drawTimes.clear();

				last_refresh = glfwGetTime();
			}

			if (glfwGetKey(windowProperties.window.get(), GLFW_KEY_R)) {
				loadRTShader();
			}

			renderPacer.tick();
		}

		glDeleteQueries(2, &timerQueries[0]);
	});

	return render_thread;
}


void App::inputHandler(std::mutex& dataLock) {
	Timer inputPacer(500);
	unsigned int timeDelta;

	while (!glfwWindowShouldClose(windowProperties.window.get())) {
		timeDelta = inputPacer.tick() / 100;

		glfwPollEvents();
		if (glfwGetKey(windowProperties.window.get(), GLFW_KEY_ESCAPE))
			glfwSetWindowShouldClose(windowProperties.window.get(), GLFW_TRUE);

		dataLock.lock();

		worldObjects.player.handleInputs(windowProperties.window);
		worldObjects.player.update(timeDelta);

		dataLock.unlock();
	}
}
