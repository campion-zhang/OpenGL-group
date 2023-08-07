#include "TextRender.h"
#include <iostream>

using namespace ftgl;

TextRender::TextRender()
{
    model_matrix = vmath::mat4::identity();
    projection_matrix = vmath::mat4::identity();
    view_matrix = vmath::mat4::identity();
}

TextRender::~TextRender()
{
    clearup();
}

bool TextRender::init(const std::string &fontfile, int size)
{
    bool ok = shader.build("../media/shaders/font/text.vert", "../media/shaders/font/text.frag");
    if (!ok)
        return false;

    atlas = texture_atlas_new(160, 15, 3);
    buffer = vertex_buffer_new("vertex:3f,tex_coord:2f,color:4f,ashift:1f,agamma:1f");
    font = texture_font_new_from_file(atlas, size, fontfile.data());

    glGenTextures(1, &atlas->id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    return true;
}

void TextRender::render(const std::string &text, int x, int y, const vmath::vec4 &color)
{
    prepare(text, x, y, color);
    oldText = text;
    oldX = x;
    oldY = y;

    glViewport(0, 0, static_cast<int>(screen_size[0]) , static_cast<int>(screen_size[1]));

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    shader.use();

    shader.setUniform1i("texture", 0);
    shader.setUniform3f("pixel", 1.0f / atlas->width, 1.0f / atlas->height, atlas->depth);

    model_matrix = vmath::translate<GLfloat>(5, screen_size[1], 0);
    shader.setUniformMatrix4fv("model", model_matrix);
    shader.setUniformMatrix4fv("view", view_matrix);
    shader.setUniformMatrix4fv("projection", projection_matrix);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas->id);
    vertex_buffer_render(buffer, GL_TRIANGLES);
    glDisable(GL_BLEND);
}

void TextRender::onResize(int width, int height)
{
    screen_size[0] = width;
    screen_size[1] = height;
    projection_matrix = vmath::ortho(0, width, 0, height, -1, 1);
}

void TextRender::clearup()
{
    if (font)
        texture_font_delete(font);
    font = nullptr;

    if (atlas) {
        glDeleteTextures(1, &atlas->id);
        atlas->id = 0;
        texture_atlas_delete(atlas);
    }
    atlas = nullptr;

    if (buffer)
        vertex_buffer_delete(buffer);
    buffer = nullptr;
}

void TextRender::prepare(const std::string &text, int x, int y, const vmath::vec4 &color)
{
    vec2 pen;
    pen.x = 0;
    pen.y = 0;
    pen.x = x;
    pen.y -= (y + font->height);
    texture_font_load_glyphs(font, text.data());

    vertex_buffer_clear(buffer);
    setText(buffer, font, text, color, &pen);

    glBindTexture(GL_TEXTURE_2D, atlas->id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, static_cast<int>(atlas->width), \
                 static_cast<int>(atlas->height), 0, GL_RGB, GL_UNSIGNED_BYTE, atlas->data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextRender::setText(vertex_buffer_t *buffer, texture_font_t *font, const std::string &text,
                         const vmath::vec4 &color, vec2 *pen)
{
    size_t i;
    float r = color[0];
    float g = color[1];
    float b = color[2];
    float a = color[3];

    char ch[1] = {0};

    for (i = 0; i < text.size(); ++i) {
        ch[0] = text[i];
        texture_glyph_t *glyph = texture_font_get_glyph(font, ch);
        if (glyph != nullptr) {
            float kerning = 0.0f;
            if (i > 0) {
                ch[0] = text[i - 1];
                kerning = texture_glyph_get_kerning( glyph, ch);
            }

            pen->x += kerning;
            int x0  = static_cast<int>( pen->x + glyph->offset_x );
            int y0  = static_cast<int>( pen->y + glyph->offset_y );
            int x1  = ( x0 + static_cast<int>(glyph->width) );
            int y1  = ( y0 - static_cast<int>(glyph->height) );
            float s0 = glyph->s0;
            float t0 = glyph->t0;
            float s1 = glyph->s1;
            float t1 = glyph->t1;
            GLuint index = static_cast<GLuint>(buffer->vertices->size);
            GLuint indices[] = {index, index + 1, index + 2, index, index + 2, index + 3};

            vertex_t vertices[] = { { static_cast<float>(x0), static_cast<float>(y0), 0,  s0, t0,  r, g, b, a, 0, 1 },
                { static_cast<float>(x0), static_cast<float>(y1), 0,  s0, t1,  r, g, b, a, 0, 1 },
                { static_cast<float>(x1), static_cast<float>(y1), 0,  s1, t1,  r, g, b, a, 0, 1 },
                { static_cast<float>(x1), static_cast<float>(y0), 0,  s1, t0,  r, g, b, a, 0, 1 }
            };

            vertex_buffer_push_back_indices( buffer, indices, 6);
            vertex_buffer_push_back_vertices( buffer, vertices, 4);
            pen->x += glyph->advance_x;
        }
    }
}
