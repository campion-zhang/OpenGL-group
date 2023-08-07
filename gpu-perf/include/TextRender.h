#ifndef TEXT_RENDER_H
#define TEXT_RENDER_H
#include <glad/glad.h>
#include <string>
#include <string.h>
#include "vmath.h"
#include "GLSLProgram.h"
#include "ftgl/platform.h"
#include "ftgl/vec234.h"
#include "ftgl/vector.h"
#include "ftgl/texture-atlas.h"
#include "ftgl/texture-font.h"
#include "ftgl/ftgl-utils.h"
#include "ftgl/vertex-buffer.h"

using namespace ftgl;

class TextRender
{
    struct vertex_t
    {
        float x,y,z;
        float u,v;
        float r,g,b,a;
        float shift,gamma;
    };
public:
    TextRender();
    ~TextRender();
public:
    bool init(const std::string& fontfile,int size);
    void render(const std::string& text,int x = 5,int y = 5,const vmath::vec4& color = vmath::vec4(1,1,1,1));
    void onResize(int width,int height);
    void clearup();
private:
    void prepare(const std::string& text,int x,int y,const vmath::vec4& color);
    void setText(vertex_buffer_t* buffer,texture_font_t* font,const std::string& text,const vmath::vec4& color,vec2* pen);
private:
    GLSLProgram shader;
    texture_font_t* font = nullptr;
    texture_atlas_t* atlas = nullptr;
    vertex_buffer_t* buffer = nullptr;
    int oldX = 0;
    int oldY = 0;
    std::string oldText;
    vmath::mat4 model_matrix,projection_matrix,view_matrix;
    vmath::vec2 screen_size;
};

#endif
