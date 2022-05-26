// Convert a tradition 3D model and its textured supported by ASSIMP into an octree.
// Optionally, view a previously converted model by changing convertMesh macro.

#include "voxgl.h"

#include "model.h"
#include "texture.h"
#include "SVO.h"
#include "DAG.h"
#include "player.h"

#include <iostream>
#include <mutex>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>



#define convertMesh 1


const int windowWidth = 1280, windowHeight = 720;
GLFWwindow* window = voxgl::createWindow("voxgl", windowWidth, windowHeight);


#if convertMesh
	const unsigned int level = 9;
	const unsigned int dimension = glm::pow(2, level);

	SVO octree(level);

	int meshToVoxels() {
		GLuint modelToVoxelsVertShader = voxgl::createShader("./shaders/modelToVoxels.vs", GL_VERTEX_SHADER);
		GLuint modelToVoxelsGeomShader = voxgl::createShader("./shaders/modelToVoxels.gs", GL_GEOMETRY_SHADER);
		GLuint modelToVoxelsFragShader = voxgl::createShader("./shaders/modelToVoxels.fs", GL_FRAGMENT_SHADER);
		std::vector<GLuint> modelToVoxelsShaders;
		modelToVoxelsShaders.emplace_back(modelToVoxelsVertShader);
		modelToVoxelsShaders.emplace_back(modelToVoxelsGeomShader);
		modelToVoxelsShaders.emplace_back(modelToVoxelsFragShader);
		GLuint modelToVoxelsProgram = voxgl::createProgram(modelToVoxelsShaders);

		GLuint voxelResolutionUniform = glGetUniformLocation(modelToVoxelsProgram, "voxelResolution");
		GLuint voxelUniform = glGetUniformLocation(modelToVoxelsProgram, "voxels");
		GLuint textureUniform = glGetUniformLocation(modelToVoxelsProgram, "diffuseTex");

		glUseProgram(modelToVoxelsProgram);
		glUniform3i(voxelResolutionUniform, dimension, dimension, dimension);
		glUniform1i(voxelUniform, 0);
		glUniform1i(textureUniform, 1);

		Model model("./assets/house.obj");

		Texture texture;
		texture.LoadTextureLinear("./assets/house.jpg");

		GLuint voxelIndex;
		glGenBuffers(1, &voxelIndex);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, voxelIndex);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, voxelIndex);

		GLuint voxelData;
		glGenBuffers(1, &voxelData);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelData);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 0x00FFFFFFu, NULL, GL_DYNAMIC_COPY);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, voxelData);

		texture.UseTexture(1);

		std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();

		model.render();

		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		std::cout << "model conversion " << (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime)).count() << "ms" << std::endl;

		texture.~Texture();
		model.~Model();
		glDeleteProgram(modelToVoxelsProgram);

		// get voxel data from gpu
		GLuint voxelCount;
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, voxelIndex);
		glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &voxelCount);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		std::cout << voxelCount << " voxels converted" << std::endl;

		std::vector<glm::uvec4> voxelVec(voxelCount, glm::uvec4(0));
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelData);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::uvec4) * voxelCount, voxelVec.data());
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		glDeleteBuffers(1, &voxelIndex);
		glDeleteBuffers(1, &voxelData);

		startTime = std::chrono::system_clock::now();

		for (unsigned int voxel = voxelCount; voxel-- > 0;) {
			octree.addElement(voxelVec.back().x, voxelVec.back().y, voxelVec.back().z, voxelVec.back().w);
			voxelVec.pop_back();
		}

		voxelVec.~vector();

		std::cout << "octree gen " << (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime)).count() << "ms" << std::endl;

		octree.save("model.svo");
		octree.printTree(false);

		return 0;
	}
#else
	// load previously converted model
	SVO octree("model.svo");
#endif




void loadRTShader(GLuint& rtProgram) {
	GLuint rtCompShader = voxgl::createShader("./shaders/naive.cs", GL_COMPUTE_SHADER);
	std::vector<GLuint> rtShaders;
	rtShaders.emplace_back(rtCompShader);
	rtProgram = voxgl::createProgram(rtShaders);
}


int raytrace() {
	std::srand(time(0));

	// create window objects
	int framebufferWidth, framebufferHeight;
	glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

	//framebufferWidth /= 2;
	//framebufferHeight /= 2;


	const GLuint work = 16;
	GLuint rtX = framebufferWidth / work + (framebufferWidth % work != 0),
		rtY = framebufferHeight / work + (framebufferHeight % work != 0);


	GLuint quadVAO;
	glGenVertexArrays(1, &quadVAO);
	glBindVertexArray(quadVAO);

	// ray trace texture
	GLuint texture;
	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framebufferWidth, framebufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);


	// quad shader
	GLuint quadVertShader = voxgl::createShader("./shaders/quad.vs", GL_VERTEX_SHADER);
	GLuint quadFragShader = voxgl::createShader("./shaders/quad.fs", GL_FRAGMENT_SHADER);
	std::vector<GLuint> quadShaders;
	quadShaders.emplace_back(quadVertShader);
	quadShaders.emplace_back(quadFragShader);
	GLuint quadProgram = voxgl::createProgram(quadShaders);

	GLuint rtProgram;
	loadRTShader(rtProgram);

	GLuint resolutionUniform = glGetUniformLocation(rtProgram, "resolution");
	GLuint camPosUniform = glGetUniformLocation(rtProgram, "cameraPos");
	GLuint camMatUniform = glGetUniformLocation(rtProgram, "cameraMat");

	glUseProgram(rtProgram);
	glUniform2i(resolutionUniform, framebufferWidth, framebufferHeight);


	// create player object
	Player player;
	player.position = glm::vec3(1.f, .5f, 1.f);
	player.camera.direction = glm::vec2(-2.f, -0.7f);

	// create octree object
	std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();

	//std::cout << "octree gen " << (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime)).count() << "ms" << std::endl;
	//octree.printTree(false);
	//octree.printReferences();


	//GLint size;
	//glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &size);
	//std::cout << "GL_MAX_SHADER_STORAGE_BLOCK_SIZE is " << size << " bytes." << std::endl;

	// octree buffer
	GLuint inner_octant_buf;
	glGenBuffers(1, &inner_octant_buf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, inner_octant_buf);
	glBufferData(GL_SHADER_STORAGE_BUFFER, octree.getInnerOctantsSize(), octree.getInnerOctantData(), GL_STATIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, inner_octant_buf);

	GLuint leaf_buffer;
	glGenBuffers(1, &leaf_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, leaf_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, octree.getLeavesSize(), octree.getLeafData(), GL_STATIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, leaf_buffer);


	std::mutex dataLock;

	glfwMakeContextCurrent(NULL);
	std::thread render_thread([&]() {
		glfwMakeContextCurrent(window);

		Timer renderPacer(60);

		GLdouble last_refresh = glfwGetTime();
		//int frames = 0;

		std::vector<unsigned int> renderTimes, drawTimes;

		while (!glfwWindowShouldClose(window)) {
			startTime = std::chrono::system_clock::now();


			// ray trace compute shader
			glUseProgram(rtProgram);


			dataLock.lock();
			glUniform3f(camPosUniform, player.camera.position.x, player.camera.position.y, player.camera.position.z);
			glUniformMatrix3fv(camMatUniform, 1, GL_FALSE, glm::value_ptr(glm::transpose(player.camera.getMatrix())));
			dataLock.unlock();

			glDispatchCompute(rtX, rtY, 1);

			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			renderTimes.push_back((std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - startTime)).count());
			startTime = std::chrono::system_clock::now();

			// draw ray trace texture to quad
			glClear(GL_COLOR_BUFFER_BIT);
			glUseProgram(quadProgram);
			glDrawArrays(GL_TRIANGLES, 0, 3);
			glfwSwapBuffers(window);

			drawTimes.push_back((std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - startTime)).count());

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

			if (glfwGetKey(window, GLFW_KEY_R)) {
				loadRTShader(rtProgram);

				resolutionUniform = glGetUniformLocation(rtProgram, "resolution");
				camPosUniform = glGetUniformLocation(rtProgram, "cameraPos");
				camMatUniform = glGetUniformLocation(rtProgram, "cameraMat");

				glUseProgram(rtProgram);
				glUniform2i(resolutionUniform, framebufferWidth, framebufferHeight);
			}

			renderPacer.tick();
		}
	});

	Timer inputPacer(500);
	unsigned int timeDelta;

	while (!glfwWindowShouldClose(window)) {
		timeDelta = inputPacer.tick()/100;

		glfwPollEvents();
		if (glfwGetKey(window, GLFW_KEY_ESCAPE))
			glfwSetWindowShouldClose(window, GLFW_TRUE);

		dataLock.lock();

		player.handleInputs(window);
		player.update(timeDelta);

		dataLock.unlock();
	}

	render_thread.join();

	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteTextures(1, &texture);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &quadVAO);

	glDeleteBuffers(1, &inner_octant_buf);
	glDeleteBuffers(1, &leaf_buffer);

	glDeleteProgram(quadProgram);
	glDeleteProgram(rtProgram);

	return 0;
}



int main()
{
#if convertMesh
	glDisable(GL_CULL_FACE);
	meshToVoxels();
#endif

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	raytrace();

	return 0;
}
