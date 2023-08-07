#version 100
attribute vec3 pos;
attribute vec4 color;

varying vec4 vertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 groundTransform;

uniform int operationEnum;
uniform int rectNum;
uniform float radiusX;
uniform float radiusY;

uniform float rotateAngle;

void main()
{
    float minX = 0.0;
    float minY = 0.0;
    float maxX = 0.0;
    float maxY = 0.0;
    bool posIsOk = false;
    int tempOperationEnum = operationEnum;
    if( tempOperationEnum == 0)
    {
        gl_Position =  groundTransform * vec4(pos, 1.0);
        posIsOk = true;
    } else if (tempOperationEnum == 1 || tempOperationEnum == 2) {
        float vertexXSigned =pos.x/abs(pos.x);
        int curRectXIndex = int(abs(pos.x) / (radiusX * 2.0)) + 1;
        maxX = float(curRectXIndex) * radiusX * 2.0 * vertexXSigned;
        minX = float(curRectXIndex - 1) * radiusX * 2.0 * vertexXSigned;
        if(maxX < minX)
        {
            float temp = maxX;
            maxX = minX;
            minX = temp;
        }

        float vertexYSigned = pos.y/abs(pos.y);
        int curRectYIndex = int(abs(pos.y) / (radiusY * 2.0)) + 1;
        maxY = float(curRectYIndex) * radiusY * 2.0 * vertexYSigned;
        minY = float(curRectYIndex - 1) * radiusY * 2.0 * vertexYSigned;
        if(maxY < minY)
        {
            float temp = maxY;
            maxY = minY;
            minY = temp;
        }

        vec3 centerPos;
        centerPos.x = ( radiusX + ( float(curRectXIndex-1)  * radiusX * 2.0) ) * vertexXSigned;
        centerPos.y = pos.y;
        centerPos.z = pos.z;

        if( (pos.x > minX && pos.x < maxX ) && (pos.y > minY && pos.y < maxY) )
        {
            vec3 transformPos;
            if(tempOperationEnum == 2)
            {
                transformPos = vec3(cos(rotateAngle) * (pos.x - centerPos.x), sin(rotateAngle) * (pos.x - centerPos.x),  0.0);
            } else {
                transformPos = vec3(cos(rotateAngle) * (pos.x - centerPos.x), 0.0, sin(rotateAngle) * (pos.x - centerPos.x) );
            }
            gl_Position = vec4(transformPos + centerPos, 1.0);
            posIsOk = true;
        }
    }
    if(!posIsOk)
    {
        gl_Position = vec4(pos, 1.0);
        posIsOk = true;
    }

    vertexColor = color;
}
