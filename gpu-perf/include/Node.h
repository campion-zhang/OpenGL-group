#ifndef NODE_H
#define NODE_H
#include <vector>
#include <functional>
#include <string.h>
#include <unistd.h>
#include <glad/glad.h>
#include <GLSLProgram.h>
#include "TextRender.h"
#include "Window.h"

#define GL_CHECK(x)                                                             \
    x;                                                                           \
    {                                                                             \
        GLenum glError = glGetError();                                             \
        if (glError != GL_NO_ERROR) {                                               \
            printf("glGetError(%s, %d) : 0x%x\n", __FILE__, __LINE__, glError);      \
        }                                                                             \
    }

#define BUFFER_OFFSET(offset) (reinterpret_cast<GLvoid*>(offset))

class Node
{
public:
    enum NodeType {
        NodeType_Generic = 0,
        NodeType_FillRate,
        NodeTypeMax
    };

    static std::string getNodeTypeStringByType(NodeType type);
    const int MIN_WEIGHT{1};
    const int MAX_WEIGHT{20};

    static constexpr double totalTime = 20.0;

    template <class T>
    inline void gl_BufferData(GLenum target, const std::vector<T> &v, GLenum usage)
    {
        glBufferData(target, v.size()*sizeof(T), &v[0], usage);
    }

    template<class T>
    inline void mapBufferDataCallback(int count, std::function<void(T *, int)> cb)
    {
        if (count <= 0 || !cb)
            return;

        int size = sizeof(T) * count;
        T *items = static_cast<T *>(malloc(size));
        cb(items, count);
        glBufferData(GL_ARRAY_BUFFER, size, items, GL_STATIC_DRAW);
        delete []items;

        /*T *items = (T *)glMapBufferRange(GL_ARRAY_BUFFER, 0, size,
                                         GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        cb(items, count);
        glUnmapBuffer(GL_ARRAY_BUFFER);*/
    }

    /* like this in shader
        layout (binding = vboIndex) uniform BLOCK
        {
            mat4 proj_matrix;
            mat4 mv_matrix[16];
        };
    */
    template<class T, int count>
    class UniformBuffer
    {
    public:
        UniformBuffer() = delete;
        UniformBuffer(std::function<void(float, T *, int)> cb, int index = 0)
        {
            /*dataUpdater = cb;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_UNIFORM_BUFFER, vbo);
            glBufferData(GL_UNIFORM_BUFFER, count * sizeof(T), NULL, GL_DYNAMIC_DRAW);
            vboIndex = index;*/
        }

        UniformBuffer(const UniformBuffer &) = delete;
        UniformBuffer &operator=(const UniformBuffer &) = delete;

        ~UniformBuffer()
        {
            glDeleteBuffers(1, &vbo);
        }
    public:
        void update(float value)
        {
            /*glBindBufferBase(GL_UNIFORM_BUFFER, vboIndex, vbo);

            T *buffer = (T *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(T),
                                              GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

            dataUpdater(value, buffer, count);
            glUnmapBuffer(GL_UNIFORM_BUFFER);*/
        }
    private:
        GLuint vbo;
        int vboIndex;
        std::function<void(float, T *, int)> dataUpdater;
    };
public:
    Node();
    Node(const Node &) = delete;
    Node &operator = (const Node &) = delete;
    virtual ~Node() = default;
public:
    virtual NodeType getNodeType();

    void setWeightValue(int weight);
    int  getWeightValue();

    int getWidth()const;
    int getHeight()const;
    float getWindowRatio()const;
public:
    enum RunningState {
        RunningState_Success,
        RunningState_Failed,
        RunningState_Abort
    };
    RunningState run();

    float getAverageFps();
    std::string getNodeName()const;

    virtual bool startup();
    virtual void render(double currentTime, double difTime);
    virtual void shutdown();
    virtual void onResized(int width, int height);

    int isExtensionSupported(const char *extension);
protected:
    void setAttribLocation(GLuint *, const char **, GLSLProgram &, int count);
protected:
    std::string nodeName;
    double startTime, endTime, lastTime = 0.0;
    float averageFps;
    unsigned int frameNum;
    PerfWindow *perfWindow = nullptr;
private:
    int width = 800;
    int height = 800;
    int weightValue = 10;
    TextRender *textRender = nullptr;
};

#endif
