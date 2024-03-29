#pragma once

#include "SVODataTypes.h"

#include <voxgl.h>
#include <model.h>
#include <texture.h>
#include <SVO.h>
#include <DAG.h>
#include <player.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>

#include <mutex>



class App
{
public:
	static App& getInstance(void) {
		static App instance;
		return instance;
	}

	App(const App&) = delete;
	App& operator=(const App&) = delete;

	~App();

	void run();


	struct AppProperties {
		bool convertMesh;
		std::string svoPath;
		std::string conversionDevice;
		std::string modelPath;
		std::string texPath;
		unsigned int octreeDepth;
		bool saveModel;
	} props;


private:
	App();

	void createWindow();

	void setupShaders();
	void setupRenderShader();

	void setupObjects();

	void loadPTShader();
	void loadRTShader();

	void loadSVO();
	void convertMeshToVoxels();

	const std::thread renderThread(std::mutex& dataLock);
	void inputHandler(std::mutex& dataLock);


	struct {
		const int windowWidth = 1280, windowHeight = 720;
		voxgl::GLFWwindow_ptr window;
	} windowProperties;


	struct {
		const GLuint rtWork = 16;	// ray cast in groups of 16x16 pixels
		GLuint rtX, rtY;

		GLuint quadVAO;
		GLuint rtTexture;

		GLuint quadProgram;
		GLuint rtProgram;

		GLuint resolutionUniform;
		GLuint camPosUniform;
		GLuint camMatUniform;
		GLuint sunDirUniform;
	} conversionShaderProperties;


	struct {
		const int fps = 60;							// rendering frame rate

		int framebufferWidth, framebufferHeight;	// screen render target dimensions
	} renderProperties;

	struct {
		const GLuint rtWork = 16;	// ray cast in groups of 16x16 pixels
		GLuint rtX, rtY;

		GLuint quadVAO;
		GLuint rtTexture;

		GLuint quadProgram;
		GLuint rtProgram;
		
		GLuint resolutionUniform;
		GLuint camPosUniform;
		GLuint camMatUniform;
		GLuint sunDirUniform;

		GLuint inner_octant_buf;
		GLuint leaf_buffer;
	} renderShaderProperties;

	struct {
		GLuint inner_octant_buf;
		GLuint leaf_buffer;
	} sharedProperties;

	struct {
		std::unique_ptr<SVO<InnerOctant, Leaf, MaskType>> octree;
		Player player;
	} worldObjects;
};

