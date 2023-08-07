#include <iostream>
#include <cstring>
#include <cassert>
#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION

struct header
{
    unsigned char       identifier[12];
    unsigned int        endianness;
    unsigned int        gltype;
    unsigned int        gltypesize;
    unsigned int        glformat;
    unsigned int        glinternalformat;
    unsigned int        glbaseinternalformat;
    unsigned int        pixelwidth;
    unsigned int        pixelheight;
    unsigned int        pixeldepth;
    unsigned int        arrayelements;
    unsigned int        faces;
    unsigned int        miplevels;
    unsigned int        keypairbytes;
};

union keyvaluepair
{
    unsigned int        size;
    unsigned char       rawbytes[4];
};


static const unsigned char identifier[] =
{
    0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
};

static const unsigned int swap32(const unsigned int u32)
{
    union
    {
        unsigned int u32;
        unsigned char u8[4];
    } a, b;

    a.u32 = u32;
    b.u8[0] = a.u8[3];
    b.u8[1] = a.u8[2];
    b.u8[2] = a.u8[1];
    b.u8[3] = a.u8[0];

    return b.u32;
}

static unsigned int calculate_stride(const header &h, unsigned int width, unsigned int pad = 4)
{
    unsigned int channels = 0;

    switch (h.glbaseinternalformat)
    {
        case GL_ALPHA:
        case GL_LUMINANCE:
        case GL_LUMINANCE_ALPHA:
            channels = 1;
            break;
        case GL_RGB:
            channels = 3;
            break;
        case GL_RGBA:
            channels = 4;
            break;
    }

    unsigned int stride = h.gltypesize * channels * width;

    stride = (stride + (pad - 1)) & ~(pad - 1);

    return stride;
}

static unsigned int calculate_face_size(const header &h)
{
    unsigned int stride = calculate_stride(h, h.pixelwidth);

    return stride * h.pixelheight;
}

GLuint Texture::loadKtxImage(const std::string &image)
{
    FILE *fp = nullptr;
    header h;
    size_t data_start, data_end;
    GLenum target = GL_NONE;

    fp = fopen(image.data(), "rb");

    if (!fp)
        return 0;

    if (fread(&h, sizeof(h), 1, fp) != 1)
        return 0;

    if (memcmp(h.identifier, identifier, sizeof(identifier)) != 0)
        return 0;

    if (h.endianness == 0x04030201)
    {
        // No swap needed
    }
    else if (h.endianness == 0x01020304)
    {
        // Swap needed
        h.endianness            = swap32(h.endianness);
        h.gltype                = swap32(h.gltype);
        h.gltypesize            = swap32(h.gltypesize);
        h.glformat              = swap32(h.glformat);
        h.glinternalformat      = swap32(h.glinternalformat);
        h.glbaseinternalformat  = swap32(h.glbaseinternalformat);
        h.pixelwidth            = swap32(h.pixelwidth);
        h.pixelheight           = swap32(h.pixelheight);
        h.pixeldepth            = swap32(h.pixeldepth);
        h.arrayelements         = swap32(h.arrayelements);
        h.faces                 = swap32(h.faces);
        h.miplevels             = swap32(h.miplevels);
        h.keypairbytes          = swap32(h.keypairbytes);
    }
    else
    {
        return 0;
    }

    // Guess target (texture type)
    if (h.pixeldepth == 0)
    {
        if (h.arrayelements == 0)
        {
            if (h.faces == 0)
            {
                target = GL_TEXTURE_2D;
            }
            else
            {
                target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
            }
        }
    }

    // Check for insanity...
    if (target == GL_NONE ||                                    // Couldn't figure out target
            (h.pixelwidth == 0) ||                                  // Texture has no width???
            (h.pixelheight == 0 && h.pixeldepth != 0))              // Texture has depth but no height???
    {
        return 0;
    }

    data_start = ftell(fp) + h.keypairbytes;
    fseek(fp, 0, SEEK_END);
    data_end = ftell(fp);
    fseek(fp, data_start, SEEK_SET);

    auto data = new unsigned char[data_end - data_start];
    memset(data, 0, data_end - data_start);
    fread(data, 1, data_end - data_start, fp);

    if (h.miplevels == 0)
        h.miplevels = 1;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(target, tex);

    switch (target)
    {
        case GL_TEXTURE_2D:
            // glTexImage2D(GL_TEXTURE_2D, 0, h.glinternalformat, h.pixelwidth, h.pixelheight, 0, h.glformat, h.gltype, data);
            if (h.gltype == GL_NONE)
            {
                glCompressedTexImage2D(GL_TEXTURE_2D, 0, h.glinternalformat, h.pixelwidth, h.pixelheight, 0, 420 * 380 / 2, data);
            }
            else
            {
                {
                    unsigned char *ptr = data;
                    unsigned int height = h.pixelheight;
                    unsigned int width = h.pixelwidth;
                    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                    for (unsigned int i = 0; i < h.miplevels; i++)
                    {
                        glTexImage2D(GL_TEXTURE_2D, i, h.glinternalformat, h.pixelwidth, h.pixelheight, 0, h.glformat, h.gltype, nullptr);
                        glTexSubImage2D(GL_TEXTURE_2D, i, 0, 0, width, height, h.glformat, h.gltype, ptr);
                        ptr += height * calculate_stride(h, width, 1);
                        height >>= 1;
                        width >>= 1;
                        if (!height)
                            height = 1;
                        if (!width)
                            width = 1;
                    }
                }
            }
            break;
        case GL_TEXTURE_CUBE_MAP:
            //glTexStorage2D(GL_TEXTURE_CUBE_MAP, h.miplevels, h.glinternalformat, h.pixelwidth, h.pixelheight);
            // glTexSubImage3D(GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0, h.pixelwidth, h.pixelheight, h.faces, h.glformat, h.gltype, data);
            {
                unsigned int face_size = calculate_face_size(h);
                for (unsigned int i = 0; i < h.faces; i++)
                {
                    glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, h.pixelwidth, h.pixelheight, h.glformat, h.gltype, data + face_size * i);
                }
            }
            break;
        default:                                               // Should never happen
            assert(1 && "bad image target");
    }

    if (h.miplevels == 1)
        glGenerateMipmap(target);

    delete [] data;
    fclose(fp);

    return tex;
}

Texture::Texture(const std::string &file)
{
    textureID = 0;
    width = 0;
    height = 0;
    bitDepth = 0;
    fileLocation = file;
}

bool Texture::loadTexture()
{
    unsigned char *data = stbi_load(fileLocation.data(), &width, &height, &bitDepth, 0);
    if(!data)
    {
        std::cout << "failed to find: " << fileLocation << "\n";
        return false;
    }

    if(bitDepth > 4 || bitDepth < 3)
    {
        return false;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int format = bitDepth == 3 ? GL_RGB : GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    return true;
}

void Texture::useTexture()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
}

void Texture::clearTexture()
{
    glDeleteTextures(1, &textureID);
    textureID = 0;
    width = 0;
    height = 0;
    bitDepth = 0;
    fileLocation = "";
}

bool Texture::loadCubeMap(const std::array<std::string, 6> &files, GLuint *texture)
{
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, *texture);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);

    for(int i = 0; i < 6; i++)
    {
        int width, height, bpp;
        unsigned char *data = stbi_load(files[i].data(), &width, &height, &bpp, 0);
        if(!data)
        {
            return false;
        }

        GLenum format = bpp == 3 ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0,
                     format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return true;
}

Texture::~Texture()
{
    clearTexture();
}
