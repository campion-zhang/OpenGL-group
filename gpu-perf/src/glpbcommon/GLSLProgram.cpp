#include <fstream>
#include <sstream>
#include <vector>
#include "Log.h"
#include "GLSLProgram.h"

std::string GLSLProgram::toString(GLSLShaderType type)
{
    if (VERTEX == type)
        return "GL_VERTEX_SHADER";
    else if (FRAGMENT == type)
        return "GL_FRAGMENT_SHADER";
    return "Undefined";
}

GLSLProgram::GLSLProgram():
    program(0)
{
}

GLSLProgram::~GLSLProgram()
{
    clear();
}

bool GLSLProgram::build(const char *vs, const char *fs)
{
    bool ok = compile(vs, fs);
    if (!ok)
        goto FLAG;

    ok = link();
    if (!ok)
        goto FLAG;

    ok = use();
    if (!ok)
        goto FLAG;

    return true;
FLAG:
    return false;
}

bool GLSLProgram::compile(const char *vs, const char *fs)
{
    return compileShader(vs, GLSLShaderType::VERTEX) && compileShader(fs, GLSLShaderType::FRAGMENT);
}

bool GLSLProgram::compileShader(const char *file, GLSLShaderType shaderType)
{
    std::ifstream stream(file);
    if (!stream) {
        Log::error("Error opening file: %s\n", file);
        return false;
    }

    std::stringstream sstream;
    sstream << stream.rdbuf();
    stream.close();

    std::string source(sstream.str());

    GLuint shader = glCreateShader(shaderType);
    if (0 == shader) {
        Log::error("unspported shader type - %s\n", toString(shaderType).data());
        return false;
    }

    const GLchar *array[] = { source.c_str() };
    glShaderSource(shader, 1, array, nullptr);

    glCompileShader(shader);

    GLint result;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if (GL_FALSE == result) {
        Log::error("shader compilation failed! ShaderType:%s\n", toString(shaderType).data());

        GLint logLen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);

        if (logLen > 0) {
            std::vector<char> log(logLen);
            GLsizei written;
            glGetShaderInfoLog(shader, logLen, &written, log.data());
            Log::error("Shader log: \n%s", log.data());
        }

        return false;
    } else {
        if (!program)
            program = glCreateProgram();
        glAttachShader(program, shader);
        glDeleteShader(shader);
        return true;
    }
}

bool GLSLProgram::link()
{
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (GL_FALSE == status) {
        Log::error("%s", "Failed to link shader program!\n");

        GLint logLen;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);

        if (logLen > 0) {
            std::vector<char> data(logLen);
            GLsizei written;
            glGetProgramInfoLog(program, logLen, &written, data.data());
            Log::error("Program log: \n%s", data.data());
        }

        return false;
    } else
        return true;
}

bool GLSLProgram::use()
{
    if (program <= 0) {
        Log::error("%s\n", "program handle is null!");
        return false;
    }

    glUseProgram(program);
    return true;
}

void GLSLProgram::clear()
{
    if (program > 0 && glIsProgram(program)) {
        glDeleteProgram(program);
        program = 0;
    }
}

void GLSLProgram::setUniform1f(const char *name, float val)
{
    GLint loc = getUniformLocation(name);
    glUniform1f(loc, val);
}

void GLSLProgram::setUniform2f(const char *name, float x, float y)
{
    GLint loc = getUniformLocation(name);
    glUniform2f(loc, x, y);
}

void GLSLProgram::setUniform2fv(const char *name, int count, const GLfloat *data)
{
    GLint loc = getUniformLocation(name);
    glUniform2fv(loc, 1, data);
}

void GLSLProgram::setUniform3f(const char *name, float v0, float v1, float v2)
{
    GLint loc = getUniformLocation(name);
    glUniform3f(loc, v0, v1, v2);
}

void GLSLProgram::setUniform4f(const char *name, float x, float y, float z, float w)
{
    GLint loc = getUniformLocation(name);
    glUniform4f(loc, x, y, z, w);
}

void GLSLProgram::setUniformMatrix3fv(const char *name, const GLfloat *m)
{
    GLint loc = getUniformLocation(name);
    glUniformMatrix3fv(loc, 1, 0, m);
}

void GLSLProgram::setUniform4fv(const char *name, const GLfloat *data)
{
    GLint loc = getUniformLocation(name);
    glUniform4fv(loc, 1, data);
}

void GLSLProgram::setUniformMatrix4fv(const char *name, const GLfloat *m)
{
    GLint loc = getUniformLocation(name);
    glUniformMatrix4fv(loc, 1, 0, m);
}

void GLSLProgram::setUniform1i(const char *name, int val)
{
    GLint loc = getUniformLocation(name);
    glUniform1i(loc, val);
}

int GLSLProgram::getUniformLocation(const char *name)
{
    std::map<std::string, int>::iterator pos;
    pos = uniformSets.find(name);

    if (pos == uniformSets.end())
        uniformSets[name] = glGetUniformLocation(program, name);
    return uniformSets[name];
}

GLuint GLSLProgram::getProgram()
{
    if (!program) {
        program = glCreateProgram();
    }
    return program;
}

GLint GLSLProgram::getAttriLocation(const char *name)
{
    GLint index = glGetAttribLocation(program, name);
    return index;
}

void GLSLProgram::bindAttriLocation(GLuint index, const char *name)
{
    glBindAttribLocation(program, index, name);
}
