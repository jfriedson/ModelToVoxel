//// Convert a traditional 3D model and its texture into an octree on the gpu with color, transparency, and normal data and rendering.
//// Optionally, view a previously converted model by changing the convertMesh macro.
//
//#include "SVODataTypes.h"
//
//#include "voxgl.h"
//
//#include "model.h"
//#include "texture.h"
//#include "SVO.h"
//#include "DAG.h"
//#include "player.h"
//
//#include <iostream>
//#include <mutex>
//#include <numeric>
//#include <random>
//#include <thread>
//#include <vector>
//
//#include <glm/glm.hpp>
//#include <glm/gtc/type_ptr.hpp>
//#include <glm/gtc/noise.hpp>
//
//
//
//#define convertMesh true
//#define modelPath "./assets/gallery.obj"
//#define texPath "./assets/gallery.jpg"
//#define saveModel true
//#define svoPath "asdf.svo"
//
//
//constexpr int windowWidth = 1280, windowHeight = 720;
//constexpr bool fullscreen = false;
//voxgl::GLFWwindow_ptr window = voxgl::createWindow("voxgl", windowWidth, windowHeight, fullscreen);
//
//
//#if convertMesh
//const unsigned int level = 11;
//const unsigned int dimension = glm::pow(2, level);
//
//SVO<InnerOctant, Leaf, MaskType> octree(level);
//
//int meshToVoxels() {
//	glfwMakeContextCurrent(window.get());
//	glClearColor(0, 0, 0, 1);
//	glClear(GL_COLOR_BUFFER_BIT);
//	glfwSwapBuffers(window.get());
//
//	GLuint modelToVoxelsVertShader = voxgl::createShader("./shaders/convertMeshToVoxels.vs", GL_VERTEX_SHADER);
//	GLuint modelToVoxelsGeomShader = voxgl::createShader("./shaders/convertOctree.gs", GL_GEOMETRY_SHADER);
//	GLuint modelToVoxelsFragShader = voxgl::createShader("./shaders/convertMeshToVoxels.fs", GL_FRAGMENT_SHADER);
//	std::vector<GLuint> modelToVoxelsShaders;
//	modelToVoxelsShaders.emplace_back(modelToVoxelsVertShader);
//	modelToVoxelsShaders.emplace_back(modelToVoxelsGeomShader);
//	modelToVoxelsShaders.emplace_back(modelToVoxelsFragShader);
//	GLuint modelToVoxelsProgram = voxgl::createProgram(modelToVoxelsShaders);
//
//	GLuint treeDepthUniform = glGetUniformLocation(modelToVoxelsProgram, "treeDepth");
//	GLuint voxelResolutionUniform = glGetUniformLocation(modelToVoxelsProgram, "voxelResolution");
//
//	glUseProgram(modelToVoxelsProgram);
//	glUniform1ui(treeDepthUniform, level);
//	glUniform1f(voxelResolutionUniform, (float)dimension);
//
//	Model model(modelPath);
//
//	Texture texture;
//	texture.LoadTextureLinear(texPath, 1);
//
//	// atomic counters
//	GLuint countersInitial[2] = { 1, 1 };
//	GLuint nodeIndicesCounter;
//	glGenBuffers(1, &nodeIndicesCounter);
//	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, nodeIndicesCounter);
//	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint) * 2, &countersInitial, GL_DYNAMIC_DRAW);
//	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, nodeIndicesCounter);
//
//	// tree nodes
//	octree.setInnerOctantsSize((0x80000000u / sizeof(InnerOctant)) - 1);
//	GLuint nodeBuffer;
//	glGenBuffers(1, &nodeBuffer);
//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodeBuffer);
//	glBufferData(GL_SHADER_STORAGE_BUFFER, 0x80000000u - sizeof(InnerOctant), octree.getInnerOctantData(), GL_DYNAMIC_COPY);
//	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, nodeBuffer);
//	octree.setInnerOctantsSize(1);
//
//	// tree leaves
//	octree.setLeavesSize((0x80000000u / sizeof(Leaf)) - 1);
//	GLuint leafBuffer;
//	glGenBuffers(1, &leafBuffer);
//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, leafBuffer);
//	glBufferData(GL_SHADER_STORAGE_BUFFER, 0x80000000u - sizeof(Leaf), octree.getLeafData(), GL_DYNAMIC_COPY);
//	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, leafBuffer);
//	octree.setLeavesSize(1);
//
//	// timing
//	GLuint timerQuery;
//	glGenQueries(1, &timerQuery);
//
//	glBeginQuery(GL_TIME_ELAPSED, timerQuery);
//
//	// conversion
//	model.render();
//	glMemoryBarrier(GL_ALL_BARRIER_BITS);
//
//	glEndQuery(GL_TIME_ELAPSED);
//	GLint queryAvailable = 0;
//	GLuint elapsed_time;
//
//	while (!queryAvailable) {
//		glGetQueryObjectiv(timerQuery,
//			GL_QUERY_RESULT_AVAILABLE,
//			&queryAvailable);
//	}
//	glGetQueryObjectuiv(timerQuery, GL_QUERY_RESULT, &elapsed_time);
//	glDeleteQueries(1, &timerQuery);
//
//
//	std::cout << "model conversion: " << elapsed_time / 1000000 << "ms\n";
//
//	texture.~Texture();
//	model.~Model();
//
//	// get voxel data from gpu
//	GLuint voxelCount[2];
//	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, nodeIndicesCounter);
//	glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint) * 2, &voxelCount);
//	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
//	glDeleteBuffers(1, &nodeIndicesCounter);
//
//	std::cout << voxelCount[0] << " elements in octree\n";
//	std::cout << voxelCount[1] << " leaves\n";
//
//	octree.setInnerOctantsSize(voxelCount[0]);
//
//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodeBuffer);
//	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(InnerOctant) * voxelCount[0], (void*)octree.getInnerOctantData());
//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
//	glDeleteBuffers(1, &nodeBuffer);
//
//	octree.setLeavesSize(voxelCount[1]);
//
//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, leafBuffer);
//	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 1, sizeof(Leaf) * voxelCount[1], (void*)octree.getLeafData());
//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
//	glDeleteBuffers(1, &leafBuffer);
//
//	glDeleteProgram(modelToVoxelsProgram);
//
//	//std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
//
//
//	//for (unsigned int voxel = 0; voxel < voxelCount; voxel++) {
//	//	glm::uvec4* const info = &voxelVec.back();
//	//	const glm::vec2 a = glm::unpackSnorm2x16(info->x);
//	//	const glm::vec2 b = glm::unpackSnorm2x16(info->y);
//	//	const glm::vec2 c = glm::unpackSnorm2x16(info->z);
//
//	//	const glm::uint32_t normal = glm::packSnorm4x8(glm::vec4(b.y, c.x, c.y, 0.f));
//
//	//	octree.addElement(glm::vec3(a.x, a.y, b.x) * glm::vec3(dimension), info->w, normal);
//
//	//	voxelVec.pop_back();
//	//	//if(voxel % 100 == 0)
//	//	//	voxelVec.shrink_to_fit();
//	//}
//
//	// stress test for SVOs. fill every voxel with a random color
//	/*for (unsigned int x = 0; x < dimension; x++)
//		for (unsigned int y = 0; y < dimension; y++)
//			for (unsigned int z = 0; z < dimension; z++)
//				octree.addElement(x, y, z, glm::packUnorm4x8(glm::vec4(0.4f)));*/
//
//				//voxelVec.~vector();
//
//				//std::cout << "octree gen: " << (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime)).count() << "ms" << std::endl;
//
//	if (saveModel)
//		octree.save(svoPath);
//
//	octree.printTree(false);
//
//	return 0;
//}
//#else
//// load previously converted model
//SVO octree(svoPath);
//#endif
//
//
//
//
//void loadRTShader(GLuint& rtProgram) {
//	GLuint rtCompShader = voxgl::createShader("./shaders/raytrace.comp", GL_COMPUTE_SHADER);
//	std::vector<GLuint> rtShaders;
//	rtShaders.emplace_back(rtCompShader);
//	rtProgram = voxgl::createProgram(rtShaders);
//}
//
//
//int raytrace() {
//	std::srand(time(0));
//
//	// create window objects
//	int framebufferWidth, framebufferHeight;
//	glfwGetFramebufferSize(window.get(), &framebufferWidth, &framebufferHeight);
//
//	//framebufferWidth /= 2;
//	//framebufferHeight /= 2;
//
//
//	const GLuint work = 16;
//	GLuint rtX = framebufferWidth / work + (framebufferWidth % work != 0),
//		rtY = framebufferHeight / work + (framebufferHeight % work != 0);
//
//
//	GLuint quadVAO;
//	glGenVertexArrays(1, &quadVAO);
//	glBindVertexArray(quadVAO);
//
//	// ray trace texture
//	GLuint texture;
//	glGenTextures(1, &texture);
//	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_2D, texture);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framebufferWidth, framebufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
//	glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
//
//
//	// quad shader
//	GLuint quadVertShader = voxgl::createShader("./shaders/quad.vs", GL_VERTEX_SHADER);
//	GLuint quadFragShader = voxgl::createShader("./shaders/quad.fs", GL_FRAGMENT_SHADER);
//	std::vector<GLuint> quadShaders;
//	quadShaders.emplace_back(quadVertShader);
//	quadShaders.emplace_back(quadFragShader);
//	GLuint quadProgram = voxgl::createProgram(quadShaders);
//
//	GLuint rtProgram;
//	loadRTShader(rtProgram);
//
//	GLuint resolutionUniform = glGetUniformLocation(rtProgram, "resolution");
//	GLuint camPosUniform = glGetUniformLocation(rtProgram, "cameraPos");
//	GLuint camMatUniform = glGetUniformLocation(rtProgram, "cameraMat");
//	GLuint sunDirUniform = glGetUniformLocation(rtProgram, "sunDir");
//
//	glUseProgram(rtProgram);
//	glUniform2f(resolutionUniform, float(framebufferWidth), float(framebufferHeight));
//	glUniform3f(sunDirUniform, 0.07f, .9f, 0.03f);
//
//
//	// create player object
//	Player player;
//	player.position = glm::vec3(-.3f, .7f, -.3f);
//	player.camera.direction = glm::vec2(1.57f, 0.f);
//
//	// create octree object
//	std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
//
//	//std::cout << "octree gen " << (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime)).count() << "ms" << std::endl;
//	//octree.printTree(false);
//	//octree.printReferences();
//
//
//	//GLint size;
//	//glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &size);
//	//std::cout << "GL_MAX_SHADER_STORAGE_BLOCK_SIZE is " << size << " bytes." << std::endl;
//
//	// octree buffer
//	GLuint inner_octant_buf;
//	glGenBuffers(1, &inner_octant_buf);
//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, inner_octant_buf);
//	glBufferData(GL_SHADER_STORAGE_BUFFER, octree.getInnerOctantsSize(), octree.getInnerOctantData(), GL_STATIC_READ);
//	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, inner_octant_buf);
//
//	GLuint leaf_buffer;
//	glGenBuffers(1, &leaf_buffer);
//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, leaf_buffer);
//	glBufferData(GL_SHADER_STORAGE_BUFFER, octree.getLeavesSize(), octree.getLeafData(), GL_STATIC_READ);
//	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, leaf_buffer);
//
//
//	std::mutex dataLock;
//
//	glfwMakeContextCurrent(NULL);
//	std::thread render_thread([&]() {
//		glfwMakeContextCurrent(window.get());
//
//		Timer renderPacer(60);
//
//		GLdouble last_refresh = glfwGetTime();
//		//int frames = 0;
//
//		std::vector<unsigned int> renderTimes, drawTimes;
//		GLuint timerQueries[2];
//		glGenQueries(2, timerQueries);
//
//		while (!glfwWindowShouldClose(window.get())) {
//			// ray trace compute shader
//			glUseProgram(rtProgram);
//
//			dataLock.lock();
//			glUniform3f(camPosUniform, player.camera.position.x, player.camera.position.y, player.camera.position.z);
//			glUniformMatrix3fv(camMatUniform, 1, GL_FALSE, glm::value_ptr(glm::transpose(player.camera.getMatrix())));
//
//			if (glfwGetKey(window.get(), GLFW_KEY_L)) {
//				glm::vec3 sunDir = -glm::normalize(player.camera.facingRay());
//				glUniform3f(sunDirUniform, sunDir.x, sunDir.y, sunDir.z);
//			}
//			dataLock.unlock();
//
//			glBeginQuery(GL_TIME_ELAPSED, timerQueries[0]);
//			glDispatchCompute(rtX, rtY, 1);
//			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
//			glEndQuery(GL_TIME_ELAPSED);
//
//
//			// draw ray trace texture to quad
//			glBeginQuery(GL_TIME_ELAPSED, timerQueries[1]);
//			glClear(GL_COLOR_BUFFER_BIT);
//			glUseProgram(quadProgram);
//			glDrawArrays(GL_TRIANGLES, 0, 3);
//			glEndQuery(GL_TIME_ELAPSED);
//
//			glfwSwapBuffers(window.get());
//
//
//			// get timer queries from opengl
//			GLint queryAvailable = 0;
//			GLuint elapsed_time;
//
//
//			queryAvailable = 0;
//			while (!queryAvailable) {
//				glGetQueryObjectiv(timerQueries[0],
//					GL_QUERY_RESULT_AVAILABLE,
//					&queryAvailable);
//			}
//			glGetQueryObjectuiv(timerQueries[0], GL_QUERY_RESULT, &elapsed_time);
//			renderTimes.push_back(elapsed_time / 1000);
//
//			queryAvailable = 0;
//			while (!queryAvailable) {
//				glGetQueryObjectiv(timerQueries[1],
//					GL_QUERY_RESULT_AVAILABLE,
//					&queryAvailable);
//			}
//			glGetQueryObjectuiv(timerQueries[1], GL_QUERY_RESULT, &elapsed_time);
//			drawTimes.push_back(elapsed_time / 1000);
//
//			//frames++;
//			if (int(glfwGetTime() - last_refresh) > 0) {
//				//std::cout << frames << "fps" << std::endl;
//				//frames = 0;
//
//				auto const renderCount = static_cast<float>(renderTimes.size());
//				auto const drawCount = static_cast<float>(drawTimes.size());
//
//				std::cout << "render time: " << std::reduce(renderTimes.begin(), renderTimes.end()) / renderCount << "us\t" << "draw time: " << std::reduce(drawTimes.begin(), drawTimes.end()) / drawCount << "us" << std::endl;
//
//				renderTimes.clear();
//				drawTimes.clear();
//
//				last_refresh = glfwGetTime();
//			}
//
//			if (glfwGetKey(window.get(), GLFW_KEY_R)) {
//				loadRTShader(rtProgram);
//
//				resolutionUniform = glGetUniformLocation(rtProgram, "resolution");
//				camPosUniform = glGetUniformLocation(rtProgram, "cameraPos");
//				camMatUniform = glGetUniformLocation(rtProgram, "cameraMat");
//				sunDirUniform = glGetUniformLocation(rtProgram, "sunDir");
//
//				glUseProgram(rtProgram);
//				glUniform2f(resolutionUniform, float(framebufferWidth), float(framebufferHeight));
//				glUniform3f(sunDirUniform, 0.07f, .9f, 0.03f);
//			}
//
//			renderPacer.tick();
//		}
//
//		glDeleteQueries(2, &timerQueries[0]);
//		});
//
//	Timer inputPacer(500);
//	unsigned int timeDelta;
//
//	while (!glfwWindowShouldClose(window.get())) {
//		timeDelta = inputPacer.tick() / 100;
//
//		glfwPollEvents();
//		if (glfwGetKey(window.get(), GLFW_KEY_ESCAPE))
//			glfwSetWindowShouldClose(window.get(), GLFW_TRUE);
//
//		dataLock.lock();
//
//		player.handleInputs(window);
//		player.update(timeDelta);
//
//		dataLock.unlock();
//	}
//
//	render_thread.join();
//
//	glBindTexture(GL_TEXTURE_2D, 0);
//	glDeleteTextures(1, &texture);
//	glBindVertexArray(0);
//	glDeleteVertexArrays(1, &quadVAO);
//
//	glDeleteBuffers(1, &inner_octant_buf);
//	glDeleteBuffers(1, &leaf_buffer);
//
//	glDeleteProgram(quadProgram);
//	glDeleteProgram(rtProgram);
//
//	return 0;
//}
//
//
//
//int main()
//{
//	glEnable(GL_CULL_FACE);
//	glCullFace(GL_BACK);
//	glDisable(GL_DEPTH_TEST);
//	glDisable(GL_BLEND);
//
//#if convertMesh
//	meshToVoxels();
//#endif
//	raytrace();
//
//	return 0;
//}
