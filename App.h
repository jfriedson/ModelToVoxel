#pragma once

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
private:
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
		std::unique_ptr<SVO> octree;
		Player player;
	} worldObjects;


public:
	struct AppProperties {
		bool convertMesh;
		std::string svoPath;
		std::string conversionDevice;
		std::string modelPath;
		std::string texPath;
		unsigned int octreeDepth;
		bool saveModel;
	} props;

	App(AppProperties& props);
	~App();

	void run();


private:
	void createWindow();

	void setupShaders();
	void setupRenderShader();

	void createObjects();

	void loadPTShader();
	void loadRTShader();

	void createSVO();
	void convertMeshToVoxels();

	const std::thread renderThread(std::mutex& dataLock);
	void inputHandler(std::mutex& dataLock);
};

