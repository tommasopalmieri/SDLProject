
/**
* Author: Tommaso Palmieri
* Assignment: Pong Clone
* Date due: 2023-10-21, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/


#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum Coordinate
{
	x_coordinate,
	y_coordinate
};

#define LOG(argument) std::cout << argument << '\n'

const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

const float BG_RED = 0.1922f,
BG_BLUE = 0.549f,
BG_GREEN = 0.9059f,
BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;
// Dimensions for paddle and ball
const float PADDLE_WIDTH = 0.3f;
const float PADDLE_HEIGHT = 1.5f;
const float BALL_SIZE = 0.3f;

// Ball speed
const float BALL_SPEED = 0.03f;
bool play_mode = true;
bool play_dir = true;

// Player 1 paddle
glm::vec3 player1_position = glm::vec3(-4.5f, 0.0f, 0.0f);
glm::vec3 player1_movement = glm::vec3(0.0f, 0.0f, 0.0f);

// Player 2 paddle
glm::vec3 player2_position = glm::vec3(4.5f, 0.0f, 0.0f);
glm::vec3 player2_movement = glm::vec3(0.0f, 0.0f, 0.0f);


// Ball
glm::vec3 ball_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 ball_movement = glm::vec3(-BALL_SPEED, BALL_SPEED, 0.0f);
float g_additional_shape_rotation = 0.0f;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const float DEGREES_PER_SECOND = 90.0f;

const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero


// move this file to where the executable is lauched. 
const char PLAYER_SPRITE_FILEPATH[] = "soph.png";
const char ADDITIONAL_SHAPE_TEXTURE_FILEPATH[] = "R.png"; 
GLuint g_additional_shape_texture_id;


glm::vec3 left_paddle_position = glm::vec3(-4.0f, 0.0f, 0.0f);
glm::vec3 right_paddle_position = glm::vec3(4.0f, 0.0f, 0.0f);

const float MIN_SIZE_CHANGE_INTERVAL = 1.0f; // 1 second, adjust as needed
const float MAX_SIZE_CHANGE_INTERVAL = 3.0f; // 3 seconds, adjust as needed
const float MIN_SCALE = 0.5f; // 50% size, adjust as needed
const float MAX_SCALE = 1.5f; // 150% size, adjust as needed



SDL_Window* g_display_window;
bool g_game_is_running = true;
bool g_is_growing = true;

ShaderProgram g_shader_program;
glm::mat4 view_matrix, m_model_matrix, m_projection_matrix, m_trans_matrix;

float m_previous_ticks = 0.0f;

GLuint g_player_texture_id;
SDL_Joystick* g_player_one_controller;

// overall position
glm::vec3 g_player_position = glm::vec3(0.0f, 0.0f, 0.0f);

// movement tracker
glm::vec3 g_player_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 shape_mov = glm::vec3(0.2f, 0.1f, 0.0f);
glm::vec3 shape_pos = glm::vec3(0.0f, 0.0f, 0.0f);


glm::vec3 g_additional_shape_position = glm::vec3(3.0f, 3.0f, 0.0f);  // setting its initial position
glm::mat4 additional_shape_model_matrix = glm::mat4(1.0f);

float get_screen_to_ortho(float coordinate, Coordinate axis)
{
	switch (axis) {
	case x_coordinate:
		return ((coordinate / WINDOW_WIDTH) * 10.0f) - (10.0f / 2.0f);
	case y_coordinate:
		return (((WINDOW_HEIGHT - coordinate) / WINDOW_HEIGHT) * 7.5f) - (7.5f / 2.0);
	default:
		return 0.0f;
	}
}

GLuint load_texture(const char* filepath)
{
	// STEP 1: Loading the image file
	int width, height, number_of_components;
	unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

	if (image == NULL)
	{
		LOG("Unable to load image. Make sure the path is correct.");
		LOG(filepath);
		assert(false);
	}

	// STEP 2: Generating and binding a texture ID to our image
	GLuint textureID;
	glGenTextures(NUMBER_OF_TEXTURES, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

	// STEP 3: Setting our texture filter parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// STEP 4: Releasing our file from memory and returning our texture id
	stbi_image_free(image);

	return textureID;
}

void initialise()
{
	// Initialise video and joystick subsystems
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

	// Open the first controller found. Returns null on error
	g_player_one_controller = SDL_JoystickOpen(0);

	g_display_window = SDL_CreateWindow("Poor Coder's Pong",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH, WINDOW_HEIGHT,
		SDL_WINDOW_OPENGL);

	SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
	SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

	g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

	m_model_matrix = glm::mat4(1.0f);
	view_matrix = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
	m_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.

	g_shader_program.set_projection_matrix(m_projection_matrix);
	g_shader_program.set_view_matrix(view_matrix);
	// Notice we haven't set our model matrix yet!

	glUseProgram(g_shader_program.get_program_id());

	glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

	g_player_texture_id = load_texture(PLAYER_SPRITE_FILEPATH);

	g_additional_shape_texture_id = load_texture(ADDITIONAL_SHAPE_TEXTURE_FILEPATH);

	// enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

bool checkCollision(const glm::vec3& a_pos, float awidth, float aheight,
	const glm::vec3& b_pos, float bwidth, float bheight) {

	return (a_pos.x - awidth / 2 < b_pos.x + bwidth / 2 &&
		a_pos.x + awidth / 2 > b_pos.x - bwidth / 2 &&
		a_pos.y - aheight / 2 < b_pos.y + bheight / 2 &&
		a_pos.y + aheight / 2 > b_pos.y - bheight / 2);
}

void shutdown()
{
	SDL_JoystickClose(g_player_one_controller);
	SDL_Quit();
}

void process_input() {
	// Reset movements
	player1_movement = glm::vec3(0.0f);
	player2_movement = glm::vec3(0.0f);

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_WINDOWEVENT_CLOSE:
		case SDL_QUIT:
			g_game_is_running = false;
			break;
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_q:
				g_game_is_running = false;
				break;
			default:
				break;
			}
			break;
		}
	}

	const Uint8* key_states = SDL_GetKeyboardState(NULL);


	if (key_states[SDL_SCANCODE_T]) {
		play_mode = !play_mode;
	}

	// Player 1 controls
	if (key_states[SDL_SCANCODE_W]) {
		player1_movement.y = 0.001f;
	}
	else if (key_states[SDL_SCANCODE_S]) {
		player1_movement.y = -0.001f;
	}

	if (play_mode) {
		// Player 2 controls
		if (key_states[SDL_SCANCODE_UP]) {
			player2_movement.y = 0.001f;
		}
		else if (key_states[SDL_SCANCODE_DOWN]) {
			player2_movement.y = -0.001f;
		}
	}

}
void update() {


	if (ball_position.x - BALL_SIZE <= -5.0f || ball_position.x + BALL_SIZE >= 5.0f) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Game Over", "This hopefully not too glitchy game is over...! ", g_display_window);
		g_game_is_running = false; // Quit game when it's over
		return;  // Exit from the update function
	}



	// Update ball position
	ball_position += ball_movement * BALL_SPEED;

	// Ball collision with top & bottom
	if (ball_position.y+ BALL_SIZE*2 > 3.75 || ball_position.y-(BALL_SIZE*2) < -3.75) {
		ball_movement.y = -ball_movement.y;
	}



	// Ball collision with paddles
	if (checkCollision(ball_position, BALL_SIZE*2, BALL_SIZE*2, player1_position, PADDLE_WIDTH * 2, PADDLE_HEIGHT) ||
		checkCollision(ball_position, BALL_SIZE*2, BALL_SIZE*2, player2_position, PADDLE_WIDTH * 2, PADDLE_HEIGHT)) {
		ball_movement.x = -ball_movement.x;
	}

	// Update paddles
	player1_position += player1_movement;

	if (play_mode) {
		player2_position += player2_movement;
	}
	else {
		if (player2_position.y - (PADDLE_HEIGHT / 2) == -3.75) {
			play_dir = false;
		}
		else if (player2_position.y + (PADDLE_HEIGHT / 2) == 3.75) {
			play_dir = true;
		}

		if (play_dir) {
			player2_position.y -= 0.01f;
		}
		else {
			player2_position.y += 0.01f;
		}
	}

	// Keep paddles within boundaries
	if (player1_position.y + (PADDLE_HEIGHT/2) > 3.75) player1_position.y = 3.75 - (PADDLE_HEIGHT / 2);
	if (player1_position.y - (PADDLE_HEIGHT / 2) < -3.75) player1_position.y = -3.75 + (PADDLE_HEIGHT / 2);
	if (player2_position.y + (PADDLE_HEIGHT / 2) > 3.75) player2_position.y = 3.75 - (PADDLE_HEIGHT / 2);
	if (player2_position.y - (PADDLE_HEIGHT / 2) < -3.75) player2_position.y = -3.75 + (PADDLE_HEIGHT / 2);
}


void draw_object(glm::mat4& object_model_matrix, GLuint object_texture_id = NULL)
{
	g_shader_program.set_model_matrix(object_model_matrix);

	if (object_texture_id != NULL) {
		glBindTexture(GL_TEXTURE_2D, object_texture_id);
	
	}

	glDrawArrays(GL_TRIANGLES, 0, 6); 
}

void render() {
	glClear(GL_COLOR_BUFFER_BIT);

	// Vertices
	glColor3f(0.0f, 0.0f, 0.0f);

	// Vertices for the paddle
	float paddle_vertices[] = {
		-PADDLE_WIDTH / 2, -PADDLE_HEIGHT / 2,
		PADDLE_WIDTH / 2, -PADDLE_HEIGHT / 2,
		PADDLE_WIDTH / 2, PADDLE_HEIGHT / 2,
		-PADDLE_WIDTH / 2, -PADDLE_HEIGHT / 2,
		PADDLE_WIDTH / 2, PADDLE_HEIGHT / 2,
		-PADDLE_WIDTH / 2, PADDLE_HEIGHT / 2
	};

	// Bind null for the texture to avoid using any texture
	glBindTexture(GL_TEXTURE_2D, 0);

	// Render Player 1
	glm::mat4 player1_model_matrix = glm::mat4(1.0f);
	player1_model_matrix = glm::translate(player1_model_matrix, player1_position);
	g_shader_program.set_model_matrix(player1_model_matrix);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, paddle_vertices);
	glEnableVertexAttribArray(0);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// Render Player 2
	glm::mat4 player2_model_matrix = glm::mat4(1.0f);
	player2_model_matrix = glm::translate(player2_model_matrix, player2_position);
	g_shader_program.set_model_matrix(player2_model_matrix);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// Set the color back to default
	glColor3f(1.0f, 1.0f, 1.0f);

	float ball_vertices[] = {
		-BALL_SIZE / 2, -BALL_SIZE / 2,
		BALL_SIZE / 2, -BALL_SIZE / 2,
		BALL_SIZE / 2, BALL_SIZE / 2,
		-BALL_SIZE / 2, -BALL_SIZE / 2,
		BALL_SIZE / 2, BALL_SIZE / 2,
		-BALL_SIZE / 2, BALL_SIZE / 2
	};

	// Vertices
	float vertices[] = {
		-0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  
		-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   
	};

	// Textures
	float texture_coordinates[] = {
		0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     
		0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     
	};

	glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(g_shader_program.get_position_attribute());

	glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
	glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());


	m_model_matrix = glm::mat4(1.0f);
	m_model_matrix = glm::translate(m_model_matrix, ball_position);
	draw_object(m_model_matrix, g_player_texture_id);  

	glDisableVertexAttribArray(g_shader_program.get_position_attribute());
	glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

	SDL_GL_SwapWindow(g_display_window);
}



int main(int argc, char* argv[])
{
	initialise();

	while (g_game_is_running)
	{
		process_input();
		update();
		render();
	}

	shutdown();
	return 0;
}