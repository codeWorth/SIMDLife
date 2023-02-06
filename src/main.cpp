#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <thread>
#include <iostream>
#include <chrono>
#include <random>

#include "simd_life.h"
#include "basic_life.h"
#include "life.h"

#define LOG_TICK

using namespace std::chrono;
using namespace std;

static const GLfloat SQUARE_VERTECIES[] = {
	-1.0f, -1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
};

bool createShader(char const* code, GLenum shaderType, GLuint& id) {
	GLuint shaderID = glCreateShader(shaderType);
	glShaderSource(shaderID, 1, &code, NULL);
	glCompileShader(shaderID);

	GLint result;
	int logLength;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0) {
		char* logInfo = new char[logLength+1];
		glGetShaderInfoLog(shaderID, logLength, NULL, logInfo);
		printf("%s\n", logInfo);
		delete[] logInfo;
		return false;
	}

	id = shaderID;
	return true;
}

GLuint createProgram(GLuint const* shaders, int count) {
	GLuint programID = glCreateProgram();
	GLint result;
	int logLength;
	
	for (int i = 0; i < count; i++) {
		glAttachShader(programID, shaders[i]);

		glGetProgramiv(programID, GL_COMPILE_STATUS, &result);
		glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 0) {
			char* logInfo = new char[logLength+1];
			glGetShaderInfoLog(shaders[i], logLength, NULL, logInfo);
			printf("%s\n", logInfo);
			delete[] logInfo;
		}
	}
	glLinkProgram(programID);

	for (int i = 0; i < count; i++) {
		glDetachShader(programID, shaders[i]);
		glDeleteShader(shaders[i]);
	}

	return programID;
}

GLuint setupShaderProgram() {
	const int shaderCount = 2;
	GLuint shaderIds[shaderCount];

	createShader( // Maps x/y pos directly to UV to draw 2d image
		"#version 330 core\n\
		layout(location = 0) in vec3 vertexPos;\
		out vec2 UV;\
		uniform float zoomAmount;\
		uniform float windowX;\
		uniform float windowY;\
		void main() {\
			gl_Position.xyz = vertexPos;\
			gl_Position.w = 1.0;\
			UV = vec2(\
				(gl_Position.x/2 + 0.5) / zoomAmount + windowX, \
				(-gl_Position.y/2 + 0.5) / zoomAmount + windowY \
			);\
		}",
		GL_VERTEX_SHADER,
		shaderIds[0]
	);
	createShader(
		"#version 330 core\n\
		in vec2 UV;\
		out vec3 color;\
		uniform sampler2D textureSampler;\
		void main() {\
			color = texture(textureSampler, UV).rgb;\
		}", 
		GL_FRAGMENT_SHADER,
		shaderIds[1]
	);

	return createProgram(shaderIds, shaderCount);

}

struct MouseData {
	bool mouseDown = false;
	double mouseX, mouseY;
	struct PanStart {
		double mouseX, mouseY;
		int windowX, windowY;
	} panStart;
};

struct FrameData {
	int x = 0;
	int y = 0;
	struct Shader {
		float windowX = 0;
		float windowY = 0;
		float zoom = 1.0f;
	} shaderData;
};

struct WindowData {
	MouseData mouseData;
	FrameData frameData;
};

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	WindowData* windowData = (WindowData *)glfwGetWindowUserPointer(window);

	windowData->mouseData.mouseX = xpos;
	windowData->mouseData.mouseY = ypos;

	if (windowData->mouseData.mouseDown) {
		double dx = -(xpos - windowData->mouseData.panStart.mouseX);
		double dy = -(ypos - windowData->mouseData.panStart.mouseY);
		dx /= windowData->frameData.shaderData.zoom;
		dy /= windowData->frameData.shaderData.zoom;
		windowData->frameData.x = windowData->mouseData.panStart.windowX + dx;
		windowData->frameData.y = windowData->mouseData.panStart.windowY + dy;
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	WindowData* windowData = (WindowData *)glfwGetWindowUserPointer(window);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS && !windowData->mouseData.mouseDown) {
			windowData->mouseData.mouseDown = true;
			windowData->mouseData.panStart.mouseX = windowData->mouseData.mouseX;
			windowData->mouseData.panStart.mouseY = windowData->mouseData.mouseY;
			windowData->mouseData.panStart.windowX = windowData->frameData.x;
			windowData->mouseData.panStart.windowY = windowData->frameData.y;
		} else if (action == GLFW_RELEASE) {
			windowData->mouseData.mouseDown = false;
		}
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	const double scrollScaling = 0.1;
	WindowData* windowData = (WindowData *)glfwGetWindowUserPointer(window);

	float oldZoom = windowData->frameData.shaderData.zoom;
	float newZoom = oldZoom * (1 + scrollScaling * yoffset);
	if (newZoom < 1) {
		newZoom = 1;
	} else if (newZoom > 32) {
		newZoom = 32;
	}
	windowData->frameData.shaderData.zoom = newZoom;

	if (newZoom == 1) {
		windowData->frameData.shaderData.windowX = 0;
		windowData->frameData.shaderData.windowY = 0;
	} else {
		windowData->frameData.shaderData.windowX += windowData->mouseData.mouseX * ((1/oldZoom) - (1/newZoom));
		windowData->frameData.shaderData.windowY += windowData->mouseData.mouseY * ((1/oldZoom) - (1/newZoom));
	}

	const float maxWindowX = WINDOW_WIDTH * (1 - (1/newZoom));
	const float maxWindowY = WINDOW_HEIGHT * (1 - (1/newZoom));
	if (windowData->frameData.shaderData.windowX < 0) {
		windowData->frameData.shaderData.windowX = 0;
	} else if (windowData->frameData.shaderData.windowX > maxWindowX) {
		windowData->frameData.shaderData.windowX = maxWindowX;
	}
	if (windowData->frameData.shaderData.windowY < 0) {
		windowData->frameData.shaderData.windowY = 0;
	} else if (windowData->frameData.shaderData.windowY > maxWindowY) {
		windowData->frameData.shaderData.windowY = maxWindowY;
	}
}

int main(int argc, char* argv[]) {

	// Drawing code is as simple as possible while still being fast here.
	// All I have is a plane that fills the entire window, and a texture on that plane
	// Then I simply write individual pixels to that texture from PIXEL_BUFFER_A
	// This seems to be plenty fast for my purposes.

	GLFWwindow* window;

	if (!glfwInit()) {
		glfwTerminate();
		return -1;
	}

	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "SIMD Life", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	gladLoadGL();
	glfwSwapInterval(1);

	GLuint shaderProgram = setupShaderProgram();
	glUseProgram(shaderProgram);
	
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_RED};
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

	GLuint vertexBuffer;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(SQUARE_VERTECIES), SQUARE_VERTECIES, GL_STATIC_DRAW);

	GLint zoomAmount = glGetUniformLocation(shaderProgram, "zoomAmount");
	GLint windowX = glGetUniformLocation(shaderProgram, "windowX");
	GLint windowY = glGetUniformLocation(shaderProgram, "windowY");

	random_device rd;
	SIMDLife* life = new SIMDLife(CELLS_WIDTH, CELLS_HEIGHT, rd);
	// BasicLife* life = new BasicLife(CELLS_SIZE, rd);
	life->setup();

	thread PHYSICS_THREAD([life]() {
		this_thread::sleep_for(milliseconds(500));	// given the window half a second to open

		high_resolution_clock timer;
		int count = 0;
		const int maxCount = 1024;

		auto t0 = timer.now();
		while (true) {
			life->tick();

			#ifdef LOG_TICK
				count++;
				if (count == maxCount) {
					long dt = duration_cast<microseconds>(timer.now() - t0).count();
					cout << (dt / maxCount) << " microsecond tick" << endl;
					count = 0;
					t0 = timer.now();
				}
			#endif

			this_thread::sleep_for(milliseconds(1));
		}
	});

	high_resolution_clock timer;
	const long millisPerFrame = 16; // 60 fps
	BYTE* pixelBuffer = (BYTE*)_mm_malloc(sizeof(BYTE)*PIXEL_COUNT, 32);

	WindowData windowData;
	glfwSetWindowUserPointer(window, &windowData);

	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0) {

		auto t0 = timer.now();

		glUniform1f(zoomAmount, windowData.frameData.shaderData.zoom);
		glUniform1f(windowX, windowData.frameData.shaderData.windowX / (float) WINDOW_WIDTH);
		glUniform1f(windowY, windowData.frameData.shaderData.windowY / (float) WINDOW_HEIGHT);
		life->draw(pixelBuffer, windowData.frameData.x, windowData.frameData.y);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, pixelBuffer);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(0);

		glfwSwapBuffers(window);
		glfwPollEvents();

		long dt = duration_cast<milliseconds>(timer.now() - t0).count(); // maintain max of 60 fps
		dt = millisPerFrame - dt;
		if (dt < 0) {
			dt = 0L;
			cout << "Not enough time to draw!" << endl;
		}
		this_thread::sleep_for(milliseconds(dt));

	}

	PHYSICS_THREAD.detach();
	delete[] pixelBuffer;
	delete life;

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}