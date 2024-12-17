#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <render/shader.h>


#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>

static GLFWwindow *window;
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
static void mouse_callback(GLFWwindow* window, double xpos, double ypos);

// OpenGL camera view parameters
static glm::vec3 eye_center;
static glm::vec3 lookat(0, 0, 0);
static glm::vec3 up(0, 1, 0);

// View control
static float viewAzimuth = 0.f;
static float viewPolar = 0.f;
static float viewDistance = 600.0f;

// Variables for cursor view control
static double lastX = 400, lastY = 300;
static float yaw = -90.0f;       //Facing negative z
static float pitch = 0.0f;
static bool firstMouse = true;




static GLuint LoadTextureTileBox(const char *texture_file_path) {
    int w, h, channels;
    uint8_t* img = stbi_load(texture_file_path, &w, &h, &channels, 3);
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // To tile textures on a box, we set wrapping to repeat
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (img) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture " << texture_file_path << std::endl;
    }
    stbi_image_free(img);

    return texture;
}










//Constructor for the 'box' that will act as a building
struct Building {
    glm::vec3 position;      //This is the position of the 'box'
    glm::vec3 scale;         // The size of the box for each axis

    GLfloat vertex_buffer_data[72] = {	// Vertex definition for a canonical box
		// Front face
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,

		// Back face
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,

		// Left face
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,

		// Right face
		1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, 1.0f,

		// Top face
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,

		// Bottom face
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,
	};

	GLfloat color_buffer_data[72] = {
		// Front, red
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		// Back, yellow
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,

		// Left, green
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		// Right, cyan
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,

		// Top, blue
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,

		// Bottom, magenta
		1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
	};

	GLuint index_buffer_data[36] = {		// 12 triangle faces of a box
		0, 1, 2,
		0, 2, 3,

		4, 5, 6,
		4, 6, 7,

		8, 9, 10,
		8, 10, 11,

		12, 13, 14,
		12, 14, 15,

		16, 17, 18,
		16, 18, 19,

		20, 21, 22,
		20, 22, 23,
	};

    // TODO: Define UV buffer data
    // ---------------------------
    // ---------------------------
    GLfloat uv_buffer_data[48] = {
    	//Front
    	0.0f, 1.0f,
    	1.0f, 1.0f,
    	1.0f, 0.0f,
    	0.0f, 0.0f,

    	//Back
    	0.0f, 1.0f,
    	1.0f, 1.0f,
    	1.0f, 0.0f,
    	0.0f, 0.0f,

    	//Left
    	0.0f, 1.0f,
    	1.0f, 1.0f,
    	1.0f, 0.0f,
    	0.0f, 0.0f,

    	//Right
    	0.0f, 1.0f,
    	1.0f, 1.0f,
    	1.0f, 0.0f,
    	0.0f, 0.0f,

    	//Top - we do not want to texture the top
    	0.0f, 0.0f,
    	0.0f, 0.0f,
    	0.0f, 0.0f,
    	0.0f, 0.0f,

    	//Bottom - we do not want to texture the bottom
    	0.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,
    };

	//The OpenGL buffers
	GLuint vertexArrayID;
	GLuint vertexBufferID;
	GLuint indexBufferID;
	GLuint colorBufferID;
	GLuint uvBufferID;
	GLuint textureID;

	//Shader IDs
	GLuint mvpMatrixID;
	GLuint textureSamplerID;
	GLuint programID;


    //Function to initalise the buildings
    void initialize(glm::vec3 position, glm::vec3 scale, const char* texturePath="../street/nightCity-facade.jpg") {

      //Define the scale of the buildings geometry
      this->position = position;
      this->scale = scale;

      //Create a Vertex array object
      glGenVertexArrays(1, &vertexArrayID);
      glBindVertexArray(vertexArrayID);

      //Create a vertex buffer object to store the vertex data
      glGenBuffers(1, &vertexBufferID);
      glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

      //Create a vertex buffer object to store the colour data
      //glGenBuffers(1, &colorBufferID);
      //glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
      //glBufferData(GL_ARRAY_BUFFER, sizeof(color_buffer_data), color_buffer_data, GL_STATIC_DRAW);

      //Create a vertex buffer object to store the UV data
      //for(int i=0; i<24; i++) uv_buffer_data[2*i+1] *=5;

      glGenBuffers(1, &uvBufferID);
      glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
      glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data, GL_STATIC_DRAW);

      //Create an index buffer object to store the index data that defines the trianlge faces
      glGenBuffers(1, &indexBufferID);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);

    	// Create and compile our GLSL program from the shaders
    	programID = LoadShadersFromFile("../street/box.vert", "../street/box.frag");
    	if (programID == 0)
    	{
    		std::cerr << "Failed to load shaders." << std::endl;
    	}

    	// Get a handle for our "MVP" uniform
    	mvpMatrixID = glGetUniformLocation(programID, "MVP");
    	textureID = LoadTextureTileBox(texturePath);
    	textureSamplerID = glGetUniformLocation(programID, "textureSampler");
    }

    //Function to render in the objects
    void render(glm::mat4 cameraMatrix) {
      glUseProgram(programID);
      glEnableVertexAttribArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

      // Model transform

      glm::mat4 modelMatrix = glm::mat4(1.0f);
      // Scale the box along each axis to make it look like a building
      modelMatrix = glm::translate(modelMatrix, position);
      modelMatrix = glm::scale(modelMatrix, scale);


      // Set model-view-projection matrix
      glm::mat4 mvp = cameraMatrix * modelMatrix;
      glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

      //  Enable UV buffer and texture sampler

      glEnableVertexAttribArray(1);
      glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

      //Set textureSampler to use texture unit 0
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, textureID);
      glUniform1i(textureSamplerID, 0);


      // Draw the box
      glDrawElements(
	  	GL_TRIANGLES,      // mode
	  	36,    			   // number of indices
	  	GL_UNSIGNED_INT,   // type
	  	(void*)0           // element array buffer offset
	  );

      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);

    }

	void cleanup() {
    	glDeleteBuffers(1, &vertexBufferID);
    	glDeleteBuffers(1, &colorBufferID);
    	glDeleteBuffers(1, &indexBufferID);
    	glDeleteVertexArrays(1, &vertexArrayID);
    	glDeleteBuffers(1, &uvBufferID);
    	glDeleteTextures(1, &textureID);
    	glDeleteProgram(programID);
    }
};



struct Floor {
	GLuint vertexArrayID;
	GLuint vertexBufferID;
	GLuint uvBufferID;
	GLuint textureID;
	GLuint programID;
	GLuint mvpMatrixID;
	GLuint textureSamplerID;

	void initialize() {
		//Vertex data for a large quad
		GLfloat vertex_buffer_data[] = {
			-1000.0f, -40.0f, -1000.0f,
			-1000.0f, -40.0f, 1000.0f,
			1000.0f, -40.0f, 1000.0f,
			1000.0f, -40.0f, -1000.0f
		};

		GLfloat uv_buffer_data[] = {
			0.0f, 0.0f,
			0.0f, 100.0f,
			100.0f, 100.0f,
			100.0f, 0.0f
		};

		glGenVertexArrays(1, &vertexArrayID);
		glBindVertexArray(vertexArrayID);

		glGenBuffers(1, &vertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

		glGenBuffers(1, &uvBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data, GL_STATIC_DRAW);

		programID = LoadShadersFromFile("../street/floor.vert", "../street/floor.frag");
		mvpMatrixID = glGetUniformLocation(programID, "MVP");
		textureID = LoadTextureTileBox("../street/road_texture.jpg");
		textureSamplerID = glGetUniformLocation(programID, "textureSampler");
	}


	void render(glm::mat4 cameraMatrix) {
		glUseProgram(programID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glm::mat4 modelMatrix = glm::mat4(1.0f);
		glm::mat4 mvp = cameraMatrix * modelMatrix;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(textureSamplerID, 0);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
	}


	void cleanup() {
		glDeleteBuffers(1, &vertexBufferID);
		glDeleteBuffers(1, &uvBufferID);
		glDeleteVertexArrays(1, &vertexArrayID);
		glDeleteTextures(1, &textureID);
		glDeleteProgram(programID);
	}
};



struct Park {
	GLuint vertexArrayID;
	GLuint vertexBufferID;
	GLuint uvBufferID;
	GLuint textureID;
	GLuint programID;
	GLuint mvpMatrixID;
	GLuint textureSamplerID;

	void initialize() {
		GLfloat vertex_buffer_data[] = {
			400.0f, -39.9f, 400.0f,    // Top right corner
			400.0f, -39.9f, 800.0f,    // Bottom right
			800.0f, -39.9f, 800.0f,    // Bottom left
			800.0f, -39.9f, 400.0f     // Top left
		};

		GLfloat uv_buffer_data[] = {
			0.0f, 0.0f,
			0.0f, 20.0f,
			20.0f, 20.0f,
			20.0f, 0.0f
		};

		glGenVertexArrays(1, &vertexArrayID);
		glBindVertexArray(vertexArrayID);

		glGenBuffers(1, &vertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

		glGenBuffers(1, &uvBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data, GL_STATIC_DRAW);

		programID = LoadShadersFromFile("../street/floor.vert", "../street/floor.frag");
		mvpMatrixID = glGetUniformLocation(programID, "MVP");
		textureID = LoadTextureTileBox("../street/grass.jpg");
		textureSamplerID = glGetUniformLocation(programID, "textureSampler");
	}

	void render(glm::mat4 cameraMatrix) {
		glUseProgram(programID);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glm::mat4 mvp = cameraMatrix * glm::mat4(1.0f);
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(textureSamplerID, 0);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
	}

	void cleanup() {
		glDeleteBuffers(1, &vertexBufferID);
		glDeleteBuffers(1, &uvBufferID);
		glDeleteVertexArrays(1, &vertexArrayID);
		glDeleteTextures(1, &textureID);
		glDeleteProgram(programID);
	}

};








int main(void) {
  // Initialise GLFW
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW." << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For MacOS
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "Lab 2", NULL, NULL);
	if (window == NULL)
	{
		std::cerr << "Failed to open a GLFW window." << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);   //Hide and capture cursor
	glfwSetCursorPosCallback(window, mouse_callback);

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetKeyCallback(window, key_callback);

	// Load OpenGL functions, gladLoadGL returns the loaded version, 0 on error.
	int version = gladLoadGL(glfwGetProcAddress);
	if (version == 0)
	{
		std::cerr << "Failed to initialize OpenGL context." << std::endl;
		return -1;
	}

	// Background
	//glClearColor(0.2f, 0.2f, 0.25f, 0.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);


	std::vector<Building> buildings;
	std::vector<Building> buildings2;

	Floor floor;
	floor.initialize();

	Park park;
	park.initialize();

	Building fence[5];
	//Front fence
	fence[0].initialize(glm::vec3(600.0f, -20.0f, 400.0f), glm::vec3(125.0f, 20.0f, 1.0f), "../street/fence.jpg");

	// Right fence
	fence[1].initialize(glm::vec3(800.0f, -20.0f, 600.0f), glm::vec3(1.0f, 20.0f, 200.0f), "../street/fence.jpg");

	// Back fence
	fence[2].initialize(glm::vec3(600.0f, -20.0f, 800.0f), glm::vec3(200.0f, 20.0f, 1.0f), "../street/fence.jpg");

	// Left fence (with gap for entrance)
	fence[3].initialize(glm::vec3(400.0f, -20.0f, 600.0f), glm::vec3(1.0f, 20.0f, 200.0f), "../street/fence.jpg");

	//fence[4].initialize(glm::vec3(400.0f, -20.0f, 200.0f), glm::vec3(125.0f, 20.0f, 1.0f), "../street/fence.jpg");


	//Create multiple buildings
	for(int i = 0; i< 5; ++i) {

			Building b;

			// Set random x and z positions, ensuring enough space between buildings
			float xPos = i * 60.0f ; // Spacing along x-axis
			float zPos = i; // Spacing along z-axis

			glm::vec3 position(xPos, 0, zPos);

			// Set random height for each building within a range
			float height = 40 ;
			glm::vec3 scale(16, height, 16);

			b.initialize(position, scale);
			buildings.push_back(b);

	}

	for (int i=0; i< 5; ++i) {
		Building b;

		float xPos = i * 60.0f ;
		float zPos = i + 100.0f;
		glm::vec3 position(xPos, 0, zPos);

		float height = 40 ;
		glm::vec3 scale(16, height, 16);
		b.initialize(position, scale);
		buildings2.push_back(b);
	}
    // ---------------------------
	



	// Camera setup
    eye_center.y = viewDistance * cos(viewPolar);
    eye_center.x = viewDistance * cos(viewAzimuth);
    eye_center.z = viewDistance * sin(viewAzimuth);

	glm::mat4 viewMatrix, projectionMatrix;
    glm::float32 FoV = 90;
	glm::float32 zNear = 0.1f;
	glm::float32 zFar = 2000.0f;
	projectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, zNear, zFar);

	do
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		viewMatrix = glm::lookAt(eye_center, lookat, up);
		glm::mat4 vp = projectionMatrix * viewMatrix;



		floor.render(vp);
		park.render(vp);
		for(int i=0; i <5; i++) {
			fence[i].render(vp);
		}

		//Render multiple buildings
		for(auto &building : buildings) {
			building.render(vp);
		}


		for (auto &building : buildings2) {
			building.render(vp);
		}

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (!glfwWindowShouldClose(window));


	for(auto &building : buildings) {
		building.cleanup();
	}

	floor.cleanup();
	park.cleanup();
	for(int i = 0; i < 5; i++) {
		fence[i].cleanup();
	}


	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}





// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	float moveSpeed = 5.0f;
	glm::vec3 direction = glm::normalize(lookat - eye_center);
	glm::vec3 right = glm::normalize(glm::cross(direction, up));

	if(key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		lookat += direction * moveSpeed;
		eye_center += direction * moveSpeed;
	}
	if(key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		lookat -= direction * moveSpeed;
		eye_center -= direction * moveSpeed;
	}
	if(key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		lookat -= right * moveSpeed;
		eye_center -= right * moveSpeed;
	}
	if(key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		lookat += right * moveSpeed;
		eye_center += right * moveSpeed;
	}

	//Close the window
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}


static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
		return;
	}

	float xoffset = float(xpos - lastX);
	float yoffset = float(lastY - ypos);
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	//Prevent flipping
	if(pitch > 89.0f) {
		pitch = 89.0f;
	}
	if(pitch < -89.0f) {
		pitch = -89.0f;
	}

	//Calculate new direction
	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	lookat = eye_center + glm::normalize(direction);
}