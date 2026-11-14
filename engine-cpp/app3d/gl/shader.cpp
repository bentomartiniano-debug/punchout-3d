// app3d/gl/shader.cpp
#include "gl/shader.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
namespace KoRama {
static GLuint compile(GLenum t, const char* src) {
    GLuint s = glCreateShader(t); glShaderSource(s, 1, &src, nullptr); glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[2048]; glGetShaderInfoLog(s, sizeof log, nullptr, log); throw std::runtime_error(std::string("shader: ") + log); }
    return s;
}
void Shader::build(const char* vs, const char* fs) {
    GLuint a = compile(GL_VERTEX_SHADER, vs), b = compile(GL_FRAGMENT_SHADER, fs);
    id_ = glCreateProgram(); glAttachShader(id_, a); glAttachShader(id_, b); glLinkProgram(id_);
    GLint ok = 0; glGetProgramiv(id_, GL_LINK_STATUS, &ok);
    if (!ok) { char log[2048]; glGetProgramInfoLog(id_, sizeof log, nullptr, log); throw std::runtime_error(std::string("link: ") + log); }
    glDeleteShader(a); glDeleteShader(b);
}
void Shader::set(const char* n, const glm::mat4& v) const { glUniformMatrix4fv(loc(n), 1, GL_FALSE, glm::value_ptr(v)); }
void Shader::set(const char* n, const glm::vec4& v) const { glUniform4fv(loc(n), 1, glm::value_ptr(v)); }
void Shader::set(const char* n, const glm::vec3& v) const { glUniform3fv(loc(n), 1, glm::value_ptr(v)); }
void Shader::set(const char* n, float v) const { glUniform1f(loc(n), v); }
void Shader::set(const char* n, int v) const { glUniform1i(loc(n), v); }
}
