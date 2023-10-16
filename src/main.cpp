#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <thread>
#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>

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

struct MousePosition {
	double x;
	double y;
};

struct WindowPosition {
	double x;
	double y;
};

struct GameState {
	bool playing = true;
	bool frameAdvance = false;
	bool shouldEnd = false;
};

struct AppData {
	bool mouseDown = false;
	float zoom = 1.0f;
	MousePosition mouse;
	WindowPosition window;
	struct {
		MousePosition mouse;
		WindowPosition window;
	} panStart;

	GameState gameState;
};

void clampWindowPosition(double& x, double& y, float zoom) {
	const double maxWindowX = CELLS_WIDTH - WINDOW_WIDTH / zoom;
	const double maxWindowY = CELLS_HEIGHT - WINDOW_HEIGHT / zoom;
	x = clamp(x, (double)0, maxWindowX);
	y = clamp(y, (double)0, maxWindowY);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	AppData* appData = (AppData *)glfwGetWindowUserPointer(window);

	appData->mouse.x = xpos;
	appData->mouse.y = ypos;

	if (appData->mouseDown) {
		double dx = -(xpos - appData->panStart.mouse.x);
		double dy = -(ypos - appData->panStart.mouse.y);
		dx /= appData->zoom;
		dy /= appData->zoom;

		appData->window.x = appData->panStart.window.x + dx;
		appData->window.y = appData->panStart.window.y + dy;
		clampWindowPosition(appData->window.x, appData->window.y, appData->zoom);
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	AppData* appData = (AppData *)glfwGetWindowUserPointer(window);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS && !appData->mouseDown) {
			appData->mouseDown = true;
			appData->panStart.mouse.x = appData->mouse.x;
			appData->panStart.mouse.y = appData->mouse.y;
			appData->panStart.window.x = appData->window.x;
			appData->panStart.window.y = appData->window.y;
		} else if (action == GLFW_RELEASE) {
			appData->mouseDown = false;
		}
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	const double scrollScaling = 0.1;
	AppData* appData = (AppData *)glfwGetWindowUserPointer(window);

	float oldZoom = appData->zoom;
	float newZoom = oldZoom * (1 + scrollScaling * yoffset);
	newZoom = clamp(newZoom, 1.f, 32.f);
	appData->zoom = newZoom;

	appData->window.x += appData->mouse.x * ((1/oldZoom) - (1/newZoom));
	appData->window.y += appData->mouse.y * ((1/oldZoom) - (1/newZoom));

	clampWindowPosition(appData->window.x, appData->window.y, appData->zoom);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	AppData* appData = (AppData *)glfwGetWindowUserPointer(window);

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        appData->gameState.playing = !appData->gameState.playing;
	}

	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
        appData->gameState.frameAdvance = true;
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		appData->gameState.playing = false;
		appData->gameState.shouldEnd = true;
	}
}

void setShaderParams(AppData& appData, GLint zoomAmount, GLint windowX, GLint windowY) {
	glUniform1f(zoomAmount, appData.zoom);

	const int maxX = CELLS_WIDTH - WINDOW_WIDTH;
	const int maxY = CELLS_HEIGHT - WINDOW_HEIGHT;

	int drawX = clamp((int) appData.window.x, 0, maxX);
	int drawY = clamp((int) appData.window.y, 0, maxY);

	double windowXVal = appData.window.x - drawX;
	double windowYVal = appData.window.y - drawY;
	const double maxWindowX = WINDOW_WIDTH * (1 - (1/appData.zoom));
	const double maxWindowY = WINDOW_HEIGHT * (1 - (1/appData.zoom));
	windowXVal = clamp(windowXVal, (double)0, maxWindowX);
	windowYVal = clamp(windowYVal, (double)0, maxWindowY);

	glUniform1f(windowX, (float) (windowXVal / WINDOW_WIDTH));
	glUniform1f(windowY, (float) (windowYVal / WINDOW_HEIGHT));
}

void drawLife(AppData& appData, SIMDLife* life, BYTE* pixelBuffer) {
	const int maxX = CELLS_WIDTH - WINDOW_WIDTH;
	const int maxY = CELLS_HEIGHT - WINDOW_HEIGHT;

	int drawX = clamp((int) appData.window.x, 0, maxX);
	int drawY = clamp((int) appData.window.y, 0, maxY);
	life->draw(pixelBuffer, drawX, drawY);
}

int main(int argc, char* argv[]) {

	// Drawing code is as simple as possible while still being fast here.
	// All I have is a plane that fills the entire window, and a texture on that plane
	// Then I simply write individual pixels to that texture from PIXEL_BUFFER_A
	// This seems to be plenty fast for my purposes.

	GLFWwindow* window;
	AppData appData;

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

	thread PHYSICS_THREAD([](SIMDLife* life, AppData& appData) {
		this_thread::sleep_for(milliseconds(500));	// given the window half a second to open

		high_resolution_clock timer;
		int count = 0;
		const int maxCount = 1024;

		auto t0 = timer.now();
		while (true) {
			if (appData.gameState.playing) {
				life->tick();
			} else if (appData.gameState.frameAdvance) {
				life->tick();
				appData.gameState.frameAdvance = false;
			}

			#ifdef LOG_TICK
				count++;
				if (count == maxCount) {
					long dt = duration_cast<microseconds>(timer.now() - t0).count();
					cout << (dt / maxCount) << " microsecond tick" << endl;
					count = 0;
					t0 = timer.now();
				}
			#endif
		}
	}, life, ref(appData));

	high_resolution_clock timer;
	const long millisPerFrame = 16; // 60 fps
	BYTE* pixelBuffer = (BYTE*)_mm_malloc(sizeof(BYTE)*PIXEL_COUNT, 32);

	glfwSetWindowUserPointer(window, &appData);

	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);
	while (!appData.gameState.shouldEnd && glfwWindowShouldClose(window) == 0) {

		auto t0 = timer.now();

		setShaderParams(appData, zoomAmount, windowX, windowY);
		drawLife(appData, life, pixelBuffer);
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