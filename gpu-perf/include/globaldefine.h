#pragma once

typedef struct _VertexFormat {
    unsigned location;
    int bitOffset;
    int componentNum;
    int stride;
    unsigned dataType;
    unsigned char isNormalized;
}VertexFormat;
