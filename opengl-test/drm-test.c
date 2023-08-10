#include <inttypes.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

drmModeConnector* FindConnector(int fd)
{
    //drmModeRes描述了计算机所有的显卡信息：connector、encoder、crtc、modes等
    drmModeRes* resources = drmModeGetResources(fd);
    if (!resources) {
        return NULL;
    }

    drmModeConnector* conn = NULL;
    int i = 0;
    for (i = 0; i < resources->count_connectors; i++)
    {
        conn = drmModeGetConnector(fd, resources->connectors[i]);
        if (conn != NULL) {
            if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
                break;
            }
            else {
                drmModeFreeConnector(conn);
            }
        }
    }
    drmModeFreeResources(resources);
    return conn;
}

int FindCrtc(int fd, drmModeConnector* conn)
{
    drmModeRes* resources = drmModeGetResources(fd);
    if (!resources) {
        fprintf(stderr, "drmModeGetResources failed\n");
        return -1;
    }

    uint32_t i, j;
    for (i = 0; i < conn->count_encoders; i++) {
        drmModeEncoder* enc = drmModeGetEncoder(fd, conn->encoders[i]);
        if (enc != NULL) {
            for (j = 0; j < resources->count_crtcs; j++) {
                //connector下连接若干encoder， 每个encoder支持若干crtc，possible_crtcs的某一位为1代表相应次序的crtc可用
                if (enc->possible_crtcs & (1 << j)) {
                    int id = resources->crtcs[j];
                    drmModeFreeEncoder(enc);
                    drmModeFreeResources(resources);
                    return id;
                }
            }
            drmModeFreeEncoder(enc);
        }
    }

    drmModeFreeResources(resources);
    return -1;
}

//绘制一张全色的图
void SetColor(uint8_t* dest, int stride, int w, int h)
{
    struct color
    {
        uint32_t r, g, b;
    };
    
    struct color ccs[] = 
    {
        {255, 0, 0},
        {0, 255, 0},
        {0, 0, 255},
        {255, 255, 0},
        {0, 255, 255},
        {255, 0, 255}
    };
    static int i = 0;
    uint32_t j, k, off;
    uint32_t r = 255;
    uint32_t g = 1;
    uint32_t b = 1;
    for (j = 0; j < h; j++) {
        for (k = 0; k < w; k++) {
            off = stride * j + k * 4;
            *(uint32_t*)&(dest[off]) = (ccs[i].r << 16) | (ccs[i].g << 8) | ccs[i].b;
        }
    }
    i++;
    printf("draw picture\n");
}

int main(int argc, char* argv[])
{
    int ret, fd;
    fd = open("/dev/dri/card0", O_RDWR | __O_CLOEXEC | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "couldn`t open %s, skipping\n", "");
        return -1;
    }

    drmSetMaster(fd);

    drmModeConnectorPtr connector = FindConnector(fd);

    int width = connector->modes[0].hdisplay;
    int height = connector->modes[0].vdisplay;
    printf("display is %d * %d.\n", width, height);

    int crtcid = FindCrtc(fd, connector);
    struct drm_mode_create_dumb creq;
    memset(&creq, 0, sizeof(creq));

    creq.width = width;
    creq.height = height;
    creq.bpp = 32;
    creq.flags = 0;
    ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
    if (0 != ret) {
        printf("create dumb failed!\n");
        return -1;
    }

    uint32_t framebuffer = -1;
    uint32_t stride = creq.pitch;
    ret = drmModeAddFB(fd, width, height, 24, 32, creq.pitch, creq.handle, &framebuffer);
    if (ret) {
        printf("failed to create fb.\n");
        return -1;
    }

    struct drm_mode_map_dumb mreq; //请求映射缓存到内存
    mreq.handle = creq.handle;
    ret = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
    if (ret) {
        printf("map dumb failed!\n");
        return -1;
    }

    uint8_t* buf = mmap(0, creq.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mreq.offset);
    if (buf == MAP_FAILED) {
        printf("mmap failed!\n");
        return -1;
    }
    memset(buf, 255, creq.size);
    ret = drmModeSetCrtc(fd, crtcid, framebuffer, 0, 0, &connector->connector_id, 1, connector->modes);
    if (ret) {
        fprintf(stderr, "failed to set mode: %m\n");
        return -1;
    }
    int cc = 0;
    while (cc < 5) {
        SetColor(buf, stride, width, height);
        drmModePageFlip(fd, crtcid, framebuffer, DRM_MODE_PAGE_FLIP_EVENT, 0);
        cc++;
        sleep(2);
    }

    getchar();
    close(fd);
    exit(0);    

    printf("123");
    return 0;
}