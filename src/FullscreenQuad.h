#pragma once
#include <glad/glad.h>

class FullScreenQuad {
public:
	FullScreenQuad() {
		static const char * vs_source[] =
		{
			"#version 420 core                                                 \n"
			"                                                                  \n"
			"out vec2 uv;                                                      \n"
			"                                                                  \n"
			"void main(void)                                                   \n"
			"{                                                                 \n"
			"    uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);           \n"
			"    gl_Position = vec4(uv * vec2(2,-2) + vec2(-1,1), 0, 1);       \n"
			"}                                                                 \n"
		};

		static const char * fs_source[] =
		{
			"#version 420 core                                                 \n"
			"in vec2 uv;                                                       \n"
			"uniform sampler2D s;                                              \n"
			"                                                                  \n"
			"out vec4 color;                                                   \n"
			"                                                                  \n"
			"void main(void)                                                   \n"
			"{                                                                 \n"
			"    vec3 texColor = texture(s, uv).rgb;                           \n"
			"    color = vec4(texColor, 1.0);                                  \n"
			"}                                                                 \n"
		};

		program = glCreateProgram();
		GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fs, 1, fs_source, NULL);
		glCompileShader(fs);
		GLint params;
		glGetShaderiv(fs, GL_COMPILE_STATUS, &params);
		if (params == GL_FALSE) {
			GLint infoLogLength;
			glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &infoLogLength);

			GLchar* strInfoLog = new GLchar[infoLogLength + 1];
			glGetShaderInfoLog(fs, infoLogLength, NULL, strInfoLog);
			printf("%s", strInfoLog);
			delete strInfoLog;
		}

		GLuint vs = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vs, 1, vs_source, NULL);
		glCompileShader(vs);
		glGetShaderiv(vs, GL_COMPILE_STATUS, &params);
		if (params == GL_FALSE) {
			GLint infoLogLength;
			glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &infoLogLength);

			GLchar* strInfoLog = new GLchar[infoLogLength + 1];
			glGetShaderInfoLog(vs, infoLogLength, NULL, strInfoLog);
			printf("%s", strInfoLog);
			delete strInfoLog;
		}

		glAttachShader(program, vs);
		glAttachShader(program, fs);

		glLinkProgram(program);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
	}

	~FullScreenQuad() {
		glDeleteVertexArrays(1, &vao);
		glDeleteProgram(program);
	}

	void draw(GLuint texture) {
		static const GLfloat green[] = { 0.0f, 0.25f, 0.0f, 1.0f };
		glClearBufferfv(GL_COLOR, 0, green);

		glUseProgram(program);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

private:
	GLuint program;
	GLuint vao;
};
