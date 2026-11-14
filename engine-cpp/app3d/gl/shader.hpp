// app3d/gl/shader.hpp
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
namespace KoRama {
class Shader {
public:
    void build(const char* vs, const char* fs);   // lança std::runtime_error com o log
    void use() const { glUseProgram(id_); }
    // nomes são sempre literais: a chave é o ponteiro, e a busca no driver só acontece na primeira vez
    GLint loc(const char* n) const { const auto it = locs_.find(n); if (it != locs_.end()) return it->second; return locs_[n] = glGetUniformLocation(id_, n); }
    void set(const char* n, const glm::mat4& v) const;
    void set(const char* n, const glm::vec4& v) const;
    void set(const char* n, const glm::vec3& v) const;
    void set(const char* n, float v) const;
    void set(const char* n, int v) const;
private:
    GLuint id_ = 0;
    mutable std::unordered_map<const char*, GLint> locs_;
};
}
