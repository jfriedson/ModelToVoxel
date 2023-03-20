//// Convert a traditional 3D model and its texture as voxels in a hashmap. An octree and material buffer are built every frame on the gpu.
//// Optionally, view a previously converted model by changing the convertMesh macro.
//
//#include "voxgl.h"
//
//#include "model.h"
//#include "texture.h"
//#include "player.h"
//#include "robin_hood.h"
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
//#define modelPath "./assets/bumblebee.obj"
//#define texPath "./assets/bumblebee.png"
//#define saveModel false
//#define svoPath "asdf.svo"
//
//
//constexpr int windowWidth = 1280, windowHeight = 720;
//constexpr bool fullscreen = false;
//GLFWwindow* window = voxgl::createWindow("voxgl", windowWidth, windowHeight, fullscreen);
//
//
//#if convertMesh
//	const unsigned int level = 5;
//	const unsigned int dimension = glm::pow(2, level);
//
//	robin_hood::unordered_flat_map<uint64_t, glm::uvec2> voxelMap;
//
//	int meshToVoxels() {
//		glfwMakeContextCurrent(window);
//		glClearColor(0, 0, 0, 1);
//		glClear(GL_COLOR_BUFFER_BIT);
//		glfwSwapBuffers(window);
//
//		GLuint modelToVoxelsVertShader = voxgl::createShader("./shaders/convertMeshToVoxels.vs", GL_VERTEX_SHADER);
//		GLuint modelToVoxelsGeomShader = voxgl::createShader("./shaders/convert.gs", GL_GEOMETRY_SHADER);
//		GLuint modelToVoxelsFragShader = voxgl::createShader("./shaders/convertMeshToVoxels.fs", GL_FRAGMENT_SHADER);
//		std::vector<GLuint> modelToVoxelsShaders;
//		modelToVoxelsShaders.emplace_back(modelToVoxelsVertShader);
//		modelToVoxelsShaders.emplace_back(modelToVoxelsGeomShader);
//		modelToVoxelsShaders.emplace_back(modelToVoxelsFragShader);
//		GLuint modelToVoxelsProgram = voxgl::createProgram(modelToVoxelsShaders);
//
//
//		GLuint voxelResolutionUniform = glGetUniformLocation(modelToVoxelsProgram, "voxelResolution");
//
//		glUseProgram(modelToVoxelsProgram);
//		glUniform1f(voxelResolutionUniform, (float)dimension);
//
//		Model model(modelPath);
//
//		Texture texture;
//		texture.LoadTextureLinear(texPath);
//		texture.UseTexture(1);
//
//		GLuint voxelIndex;
//		glGenBuffers(1, &voxelIndex);
//		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, voxelIndex);
//		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_STREAM_READ);
//		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, voxelIndex);
//
//		GLuint voxelBuffer;
//		glGenBuffers(1, &voxelBuffer);
//		glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelBuffer);
//		glBufferData(GL_SHADER_STORAGE_BUFFER, 0x80000000u - sizeof(glm::uvec4), NULL, GL_STREAM_READ);
//		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, voxelBuffer);
//
//		// timing
//		GLuint timerQuery;
//		glGenQueries(1, &timerQuery);
//
//		glBeginQuery(GL_TIME_ELAPSED, timerQuery);
//
//		model.render();
//
//		glMemoryBarrier(GL_ALL_BARRIER_BITS);
//
//		glEndQuery(GL_TIME_ELAPSED);
//		GLint queryAvailable = 0;
//		GLuint elapsed_time;
//
//		while (!queryAvailable) {
//			glGetQueryObjectiv(timerQuery,
//				GL_QUERY_RESULT_AVAILABLE,
//				&queryAvailable);
//		}
//		glGetQueryObjectuiv(timerQuery, GL_QUERY_RESULT, &elapsed_time);
//
//
//		std::cout << "model conversion: " << elapsed_time / 1000000 << "ms\n";
//
//		texture.~Texture();
//		model.~Model();
//
//		// get voxel data from gpu
//		GLuint voxelCount;
//		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, voxelIndex);
//		glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &voxelCount);
//		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
//
//		std::cout << voxelCount << " voxels converted\n";
//
//		std::vector<glm::uvec4> voxelVec(voxelCount);
//		glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelBuffer);
//		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::uvec4) * voxelCount, voxelVec.data());
//		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
//
//		glMemoryBarrier(GL_ALL_BARRIER_BITS);
//
//		glDeleteBuffers(1, &voxelIndex);
//		glDeleteBuffers(1, &voxelBuffer);
//
//		glDeleteQueries(1, &timerQuery);
//
//		glDeleteProgram(modelToVoxelsProgram);
//
//
//		std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
//
//		//voxelMap.reserve(voxelCount);
//
//		for (unsigned int i = voxelVec.size(); i > 0; i--) {
//			const glm::uvec4 info = voxelVec.back(); voxelVec.pop_back();
//
//			const uint64_t pos = (uint64_t(info.x) << 32) | uint64_t(info.y);
//
//			voxelMap.try_emplace(pos, glm::uvec2(info.z, info.w));
//		}
//
//		voxelVec.~vector();
//
//
//
//		std::cout << "map gen: " << (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime)).count() << "ms" << std::endl;
//
//		/*if (saveModel)
//			octree.save(svoPath);
//
//		octree.printTree(false);*/
//
//		return 0;
//	}
//#else
//	// load previously converted model
//	SVO octree(svoPath);
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
//	glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
//
//	//framebufferWidth /= 2;
//	//framebufferHeight /= 2;
//
//
//	const GLuint work = 16;
//	GLuint rtX = framebufferWidth  / work + (framebufferWidth % work != 0),
//		   rtY = framebufferHeight / work + (framebufferHeight % work != 0);
//
//
//	GLuint quadVAO;
//	glGenVertexArrays(1, &quadVAO);
//	glBindVertexArray(quadVAO);
//
//	// ray trace texture
//	GLuint rtTexture;
//	glGenTextures(1, &rtTexture);
//	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_2D, rtTexture);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framebufferWidth, framebufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
//	glBindImageTexture(0, rtTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
//
//	// voxel streaming request texture
//	GLuint streamReq;
//	glGenTextures(1, &streamReq);
//	glActiveTexture(GL_TEXTURE1);
//	glBindTexture(GL_TEXTURE_2D, streamReq);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framebufferWidth, framebufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
//	glBindImageTexture(1, streamReq, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
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
//	//GLint size;
//	//glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &size);
//	//std::cout << "GL_MAX_SHADER_STORAGE_BLOCK_SIZE is " << size << " bytes." << std::endl;
//
//	std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
//
//	// octree buffer
//	GLuint inner_octant_buf;
//	glGenBuffers(1, &inner_octant_buf);
//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, inner_octant_buf);
//	glBufferData(GL_SHADER_STORAGE_BUFFER, 0x80000000 - sizeof(SVO::I), NULL, GL_DYNAMIC_READ);
//	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, inner_octant_buf);
//
//	GLuint leaf_buffer;
//	glGenBuffers(1, &leaf_buffer);
//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, leaf_buffer);
//	glBufferData(GL_SHADER_STORAGE_BUFFER, 0x80000000, NULL, GL_DYNAMIC_READ);
//	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, leaf_buffer);
//
//
//	std::mutex dataLock;
//
//	glfwMakeContextCurrent(NULL);
//	std::thread render_thread([&]() {
//		glfwMakeContextCurrent(window);
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
//		while (!glfwWindowShouldClose(window)) {
//			// ray trace compute shader
//			glUseProgram(rtProgram);
//
//			dataLock.lock();
//			glUniform3f(camPosUniform, player.camera.position.x, player.camera.position.y, player.camera.position.z);
//			glUniformMatrix3fv(camMatUniform, 1, GL_FALSE, glm::value_ptr(glm::transpose(player.camera.getMatrix())));
//
//			if (glfwGetKey(window, GLFW_KEY_L)) {
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
//			glfwSwapBuffers(window);
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
//			if (glfwGetKey(window, GLFW_KEY_R)) {
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
//	});
//
//	Timer inputPacer(500);
//	unsigned int timeDelta;
//
//	while (!glfwWindowShouldClose(window)) {
//		timeDelta = inputPacer.tick()/100;
//
//		glfwPollEvents();
//		if (glfwGetKey(window, GLFW_KEY_ESCAPE))
//			glfwSetWindowShouldClose(window, GLFW_TRUE);
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
//	glDeleteTextures(1, &rtTexture);
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
