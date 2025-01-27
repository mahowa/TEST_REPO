#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#define M_PI 3.141592654f

unsigned int g_windowWidth = 600;
unsigned int g_windowHeight = 600;
char* g_windowName = "HW2-Transform-Coding-Image";

#define IMAGE_FILE "data/cameraman.ppm"
//#define IMAGE_FILE "data/mandi.ppm"
//#define IMAGE_FILE "data/moon.ppm"

GLFWwindow* g_window;

int g_image_width;
int g_image_height;

std::vector<float> g_luminance_data;
std::vector<float> g_compressed_luminance_data;
float* q = new float[8 * 8];;

struct color
{
	unsigned char r, g, b;
};

bool g_draw_origin = true;

// auxiliary math functions
float dotProduct(const float* a, const float* b, int size)
{
	float sum = 0;
	for (int i = 0; i < size; i++)
	{
		sum += a[i] * b[i];
	}
	return sum;
}

void normalize(float* a, int size)
{
	float len = 0;
	for (int i = 0; i < size; i++)
	{
		len += a[i] * a[i];
	}
	len = sqrt(len);
	for (int i = 0; i < size; i++)
	{
		a[i] = a[i] / len;
	}
}

// input (size x 1) vector a and b, output (size x size ) outerProduct matrix r
void outerProduct(const float* a, const float* b, float* r, int size)
{
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			r[i * size + j] = a[i] * b[j];
		}
	}
}


void DCTvector(int N, int k, float* q)
{
	// TODO: part of Homework Task 1
	// generate vector q, which is defined in equation (1)

	for (int i = 0; i < N; i++){
		q[i] = cos((M_PI / 16)*(k)*(2 * i + 1));
	}
}

void CompressBlock(const float* A, float* B, int m)
{
	// TODO: Homework Task 2 (see the PDF description)
	float C[64];

	///Compute DCT
	for (int i = 0; i < 8; i++){
		for (int j = 0; j < 8; j++){
			///Compress
			if (i + j > m){
				C[i * 8 + j] = 0;
				continue;
			}

			float R[64];
			outerProduct(q + (i * 8), q + (j * 8), R, 8);
			C[i * 8 + j] = dotProduct(R,A, 64);
		}
	}
	
	
	for (int i = 0; i < 8; i++){
		for (int j = 0; j < 8; j++){
			float R[64];
			outerProduct(q + (i * 8), q + (j * 8), R, 8);
			for (int k = 0; k < 8; k++){
				for (int t = 0; t < 8; t++){
					B[k * 8 + t] += R[k * 8 + t] * C[i * 8 + j];
				}
			}
		}
	}



}


void CompressImage(const std::vector<float> I, std::vector<float>& O, int m)
{
	float tempBlock[64];

	// TODO: Homework Task 2 (see the PDF description)
	for (int i = 0; i < g_image_height / 8; i++){
		for (int j = 0; j < g_image_width / 8; j++){
			// traverse each element in the block
			for (int k = 0; k < 8; k++){
				for (int t = 0; t < 8; t++){
					tempBlock[k * 8 + t] = I.at((i * 8 + k)*g_image_width + j * 8 + t);
				}
			}
			//initialize b
			float B[64];
			for (int ii = 0; ii < 64; ii++){
				B[ii] = 0;
			}

			//Compress the block
			CompressBlock(tempBlock, B, m);

			for (int k = 0; k < 8; k++){
				for (int t = 0; t < 8; t++){
					//Compile output image (add ot output image)
					O.at((i * 8 + k)*g_image_width + j * 8 + t) = B[k * 8 + t];
				}
			}
		}
	}
	 
}

int ReadLine(FILE *fp, int size, char *buffer)
{
	int i;
	for (i = 0; i < size; i++) {
		buffer[i] = fgetc(fp);
		if (feof(fp) || buffer[i] == '\n' || buffer[i] == '\r') {
			buffer[i] = '\0';
			return i + 1;
		}
	}
	return i;
}

//-------------------------------------------------------------------------------

bool LoadPPM(FILE *fp, int &width, int &height, std::vector<color> &data)
{
	const int bufferSize = 1024;
	char buffer[bufferSize];
	ReadLine(fp, bufferSize, buffer);
	if (buffer[0] != 'P' && buffer[1] != '6') return false;

	ReadLine(fp, bufferSize, buffer);
	while (buffer[0] == '#') ReadLine(fp, bufferSize, buffer);  // skip comments

	sscanf(buffer, "%d %d", &width, &height);

	ReadLine(fp, bufferSize, buffer);
	while (buffer[0] == '#') ReadLine(fp, bufferSize, buffer);  // skip comments

	data.resize(width*height);
	fread(data.data(), sizeof(color), width*height, fp);

	return true;
}

void glfwErrorCallback(int error, const char* description)
{
	std::cerr << "GLFW Error " << error << ": " << description << std::endl;
	exit(1);
}

void glfwKeyCallback(GLFWwindow* p_window, int p_key, int p_scancode, int p_action, int p_mods)
{
	if (p_key == GLFW_KEY_ESCAPE && p_action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(g_window, GL_TRUE);
	}
	else if (p_action == GLFW_PRESS)
	{
		switch (p_key)
		{
		case 49:	// press '1'
			g_draw_origin = true;
			break;
		case 50:	// press '2'
			g_draw_origin = false;
			break;
		default:
			break;
		}
	}
}

void initWindow()
{
	// initialize GLFW
	glfwSetErrorCallback(glfwErrorCallback);
	if (!glfwInit())
	{
		std::cerr << "GLFW Error: Could not initialize GLFW library" << std::endl;
		exit(1);
	}

	g_window = glfwCreateWindow(g_windowWidth, g_windowHeight, g_windowName, NULL, NULL);
	if (!g_window)
	{
		glfwTerminate();
		std::cerr << "GLFW Error: Could not initialize window" << std::endl;
		exit(1);
	}

	// callbacks
	glfwSetKeyCallback(g_window, glfwKeyCallback);

	// Make the window's context current
	glfwMakeContextCurrent(g_window);

	// turn on VSYNC
	glfwSwapInterval(1);
}

void initGL()
{
	glClearColor(1.f, 1.f, 1.f, 1.0f);
}

void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (g_draw_origin)
		glDrawPixels(g_image_width, g_image_height, GL_LUMINANCE, GL_FLOAT, &g_luminance_data[0]);
	else
		glDrawPixels(g_image_width, g_image_height, GL_LUMINANCE, GL_FLOAT, &g_compressed_luminance_data[0]);
}

void renderLoop()
{
	while (!glfwWindowShouldClose(g_window))
	{
		// clear buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		render();

		// Swap front and back buffers
		glfwSwapBuffers(g_window);

		// Poll for and process events
		glfwPollEvents();
	}
}

bool loadImage()
{
	std::vector<color> g_image_data;
	g_image_data.clear();
	g_image_width = 0;
	g_image_height = 0;
	FILE *fp = fopen(IMAGE_FILE, "rb");
	if (!fp) return false;

	bool success = false;
	success = LoadPPM(fp, g_image_width, g_image_height, g_image_data);

	g_luminance_data.resize(g_image_width * g_image_height);
	g_compressed_luminance_data.resize(g_image_width * g_image_height);
	for (int i = 0; i < g_image_height; i++)
	{
		for (int j = 0; j < g_image_width; j++)
		{
			// the index are not matching because of the difference between image space and OpenGl screen space
			g_luminance_data[i* g_image_width + j] = g_image_data[(g_image_height - i - 1)* g_image_width + j].r / 255.0f;
		}
	}

	g_windowWidth = g_image_width;
	g_windowHeight = g_image_height;

	return success;
}

bool writeImage()
{
	std::vector<color> tmpData;
	tmpData.resize(g_image_width * g_image_height);

	for (int i = 0; i < g_image_height; i++)
	{
		for (int j = 0; j < g_image_width; j++)
		{
			// make sure the value will not be larger than 1 or smaller than 0, which might cause problem when converting to unsigned char
			float tmp = g_compressed_luminance_data[i* g_image_width + j];
			if (tmp < 0.0f)	tmp = 0.0f;
			if (tmp > 1.0f)	tmp = 1.0f;

			tmpData[(g_image_height - i - 1)* g_image_width + j].r = unsigned char(tmp * 255.0);
			tmpData[(g_image_height - i - 1)* g_image_width + j].g = unsigned char(tmp * 255.0);
			tmpData[(g_image_height - i - 1)* g_image_width + j].b = unsigned char(tmp * 255.0);
		}
	}

	FILE *fp = fopen("data/out.ppm", "wb");
	if (!fp) return false;

	fprintf(fp, "P6\r");
	fprintf(fp, "%d %d\r", g_image_width, g_image_height);
	fprintf(fp, "255\r");
	fwrite(tmpData.data(), sizeof(color), g_image_width * g_image_height, fp);

	return true;
}

int main()
{
	// create a collection of 8 orthonormal vectors
	for (int k = 0; k < 8; k++)
	{
		DCTvector(8, k, q + k * 8);
		// normalize your output vector to unit length
		normalize(q + k * 8, 8);
	}


	loadImage();

	int n = 1;	// T3ODO: change the parameter n from 1 to 16 to see different image quality
	CompressImage(g_luminance_data, g_compressed_luminance_data, n);	

	writeImage();

	// render loop
	initWindow();
	initGL();
	renderLoop();

	return 0;
}
