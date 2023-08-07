mesa 设置OpenGL ES 2.0运行版本：
    export  MESA_GLES_VERSION_OVERRIDE=2.0
    export  MESA_GLSL_VERSION_OVERRIDE=100
    
打开assimp的-Werror:
    使用cmake -DOPENWERROR=1
    
使用自定义的egl和gles库
    cmake ../ -DCUSTOM_EGL_PATH=库路径文件夹名
    或者
    cmake ../ -DCUSTOM_EGL_FILE=EGL全路径(包括文件名) -DCUSTOM_GLES_FILE=GLES全路径(包括文件名)
    
使用glfw创建窗口
    cmake ../ -DUSE_GLFW=1
    
使用egl创建窗口
    cmake ../ -DUSE_XEGL=1

使用指定的egl创建窗口
    cmake ../ -DCUSTOM_EGL_PATH=库路径文件夹名  -DCUSTOM_INCLUDE_PATH=egl和gles头文件路径
