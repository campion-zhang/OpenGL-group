#pragma once
#include <array>
#include <string>
#include <glad/glad.h>
#include "stb_image.h"

class Texture
{
public:
    static GLuint loadKtxImage(const std::string &image);
    static bool loadCubeMap(const std::array<std::string, 6> &textureNameList, GLuint *texture);

    Texture(const std::string &file = std::string());
    ~Texture();
public:
    bool loadTexture();
    void useTexture();
    void clearTexture();
private:
    GLuint textureID;
    int width, height, bitDepth;
    std::string fileLocation;
};

