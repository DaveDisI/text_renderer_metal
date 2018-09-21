 #pragma once
 
 #include <math.h>
union vec2;
union vec3;
union vec4;
union quat;
union mat4;

static vec3 cross(vec3, vec3);
static vec3 getForwardVector(mat4);
static vec3 getUpVector(mat4);
static vec3 getRightVector(mat4);
static quat rotationToQuat(vec3, float);
static quat multiply(quat, quat);
static mat4 multiply(mat4, mat4);
static mat4 genIdentityMatrix();

union vec2 {
    float v[2];
    struct{
        float x, y;
    };

    vec2(){}

    vec2(float xv, float yv): 
    x(xv), y(yv){}
};

union vec3 {
    float v[3];
    struct{
        float x, y, z;
    };

    vec3(){}

    vec3(float xv, float yv, float zv): 
    x(xv), y(yv), z(zv){}

    void normalize(){
        float length = sqrt(x * x + y * y + z * z);
        x /= length;
        y /= length;
        z /= length;
    }

    vec3 operator-(){
        return vec3(-x, -y, -z);
    }

    void operator+=(vec3 v){
        x += v.x; y += v.y; z += v.z; 
    }
    void operator-=(vec3 v){
        x -= v.x; y -= v.y; z -= v.z; 
    }
};

union vec4 {
    float v[4];
    struct {
        float x, y, z, w;
    };

    vec4(){}

    vec4(float xv, float yv, float zv, float wv): 
    x(xv), y(yv), z(zv), w(wv){}
};

union quat {
    float v[4];
    struct {
        float x, y, z, w;
    };

    quat(){}
    
    quat(float xv, float yv, float zv, float wv): 
    x(xv), y(yv), z(zv), w(wv){}

    void normalize(){
        float length = sqrt((x * x) + (y * y) + (z * z) + (w * w));
        if(length == 0){
            x = 0, y = 0, z = 0, w = 0;
        }else{
            x /= length;
            y /= length;
            z /= length;
            w /= length;
        }
    }

    void rotate(vec3 angle, float degrees){
        quat q = rotationToQuat(angle, degrees);
        quat r = multiply(*this, q);
        x = r.x; y = r.y; z = r.z; w = r.w;
    }
};

union mat4 {
    float m[4][4];

    void setIdentity(){
        m[0][0] = 1; m[1][0] = 0; m[2][0] = 0; m[3][0] = 0;
        m[0][1] = 0; m[1][1] = 1; m[2][1] = 0; m[3][1] = 0;
        m[0][2] = 0; m[1][2] = 0; m[2][2] = 1; m[3][2] = 0;
        m[0][3] = 0; m[1][3] = 0; m[2][3] = 0; m[3][3] = 1;
    }


    void translate(vec3 v){
        m[3][0] += v.x;
        m[3][1] += v.y;
        m[3][2] += v.z;
    }
};

vec3 cross(vec3 v1, vec3 v2){
    vec3 c;
    c.x = (v1.y * v2.z) - (v1.z * v2.y);
    c.y = (v1.z * v2.x) - (v1.x * v2.z);
    c.z = (v1.x * v2.y) - (v1.y * v2.x);
    return c;
}

vec3 getForwardVector(mat4 m){
    vec3 forward = vec3(-m.m[0][2], -m.m[1][2], -m.m[2][2]);
    forward.normalize();
    return forward;
}

vec3 getUpVector(mat4 m){
    vec3 up = vec3(m.m[0][1], m.m[1][1], m.m[2][1]);
    up.normalize();
    return up;
}

vec3 getRightVector(mat4 m){
    vec3 right = vec3(m.m[0][0], m.m[1][0], m.m[2][0]);
    right.normalize();
    return right;
}

quat rotationToQuat(vec3 axis, float angle){
    float hang = angle / 2.0f;
    quat q;
    q.x = axis.x * sin(hang);
    q.y = axis.y * sin(hang);
    q.z = axis.z * sin(hang);
    q.w = cos(hang);
    q.normalize();
    return q;
}

quat multiply(quat q1, quat q2){
    quat q;
    q.x =   q1.x * q2.w + q1.y * q2.z - q1.z * q2.y + q1.w * q2.x;
    q.y =  -q1.x * q2.z + q1.y * q2.w + q1.z * q2.x + q1.w * q2.y;
    q.z =   q1.x * q2.y - q1.y * q2.x + q1.z * q2.w + q1.w * q2.z;
    q.w =  -q1.x * q2.x - q1.y * q2.y - q1.z * q2.z + q1.w * q2.w;
    return q;
}

mat4 multiply(mat4 m1, mat4 m2){
    mat4 mat;

    mat.m[0][0] = (m1.m[0][0] * m2.m[0][0]) + (m1.m[1][0] * m2.m[0][1]) + (m1.m[2][0] * m2.m[0][2]) + (m1.m[3][0] * m2.m[0][3]);
    mat.m[0][1] = (m1.m[0][1] * m2.m[0][0]) + (m1.m[1][1] * m2.m[0][1]) + (m1.m[2][1] * m2.m[0][2]) + (m1.m[3][1] * m2.m[0][3]);
    mat.m[0][2] = (m1.m[0][2] * m2.m[0][0]) + (m1.m[1][2] * m2.m[0][1]) + (m1.m[2][2] * m2.m[0][2]) + (m1.m[3][2] * m2.m[0][3]);
    mat.m[0][3] = (m1.m[0][3] * m2.m[0][0]) + (m1.m[1][3] * m2.m[0][1]) + (m1.m[2][3] * m2.m[0][2]) + (m1.m[3][3] * m2.m[0][3]);

    mat.m[1][0] = (m1.m[0][0] * m2.m[1][0]) + (m1.m[1][0] * m2.m[1][1]) + (m1.m[2][0] * m2.m[1][2]) + (m1.m[3][0] * m2.m[1][3]);
    mat.m[1][1] = (m1.m[0][1] * m2.m[1][0]) + (m1.m[1][1] * m2.m[1][1]) + (m1.m[2][1] * m2.m[1][2]) + (m1.m[3][1] * m2.m[1][3]);
    mat.m[1][2] = (m1.m[0][2] * m2.m[1][0]) + (m1.m[1][2] * m2.m[1][1]) + (m1.m[2][2] * m2.m[1][2]) + (m1.m[3][2] * m2.m[1][3]);
    mat.m[1][3] = (m1.m[0][3] * m2.m[1][0]) + (m1.m[1][3] * m2.m[1][1]) + (m1.m[2][3] * m2.m[1][2]) + (m1.m[3][3] * m2.m[1][3]);

    mat.m[2][0] = (m1.m[0][0] * m2.m[2][0]) + (m1.m[1][0] * m2.m[2][1]) + (m1.m[2][0] * m2.m[2][2]) + (m1.m[3][0] * m2.m[2][3]);
    mat.m[2][1] = (m1.m[0][1] * m2.m[2][0]) + (m1.m[1][1] * m2.m[2][1]) + (m1.m[2][1] * m2.m[2][2]) + (m1.m[3][1] * m2.m[2][3]);
    mat.m[2][2] = (m1.m[0][2] * m2.m[2][0]) + (m1.m[1][2] * m2.m[2][1]) + (m1.m[2][2] * m2.m[2][2]) + (m1.m[3][2] * m2.m[2][3]);
    mat.m[2][3] = (m1.m[0][3] * m2.m[2][0]) + (m1.m[1][3] * m2.m[2][1]) + (m1.m[2][3] * m2.m[2][2]) + (m1.m[3][3] * m2.m[2][3]);

    mat.m[3][0] = (m1.m[0][0] * m2.m[3][0]) + (m1.m[1][0] * m2.m[3][1]) + (m1.m[2][0] * m2.m[3][2]) + (m1.m[3][0] * m2.m[3][3]);
    mat.m[3][1] = (m1.m[0][1] * m2.m[3][0]) + (m1.m[1][1] * m2.m[3][1]) + (m1.m[2][1] * m2.m[3][2]) + (m1.m[3][1] * m2.m[3][3]);
    mat.m[3][2] = (m1.m[0][2] * m2.m[3][0]) + (m1.m[1][2] * m2.m[3][1]) + (m1.m[2][2] * m2.m[3][2]) + (m1.m[3][2] * m2.m[3][3]);
    mat.m[3][3] = (m1.m[0][3] * m2.m[3][0]) + (m1.m[1][3] * m2.m[3][1]) + (m1.m[2][3] * m2.m[3][2]) + (m1.m[3][3] * m2.m[3][3]);

    return mat;
}

mat4 quatToMat4(quat q){
    q.normalize();

    mat4 m;

    m.m[0][0] = 1 - (2 * (q.y * q.y)) - (2 * (q.z * q.z));
    m.m[0][1] = (2 * q.x * q.y) + (2 * q.z * q.w);
    m.m[0][2] = (2 * q.x * q.z) - (2 * q.y * q.w);
    m.m[0][3] = 0;

    m.m[1][0] = (2 * q.x * q.y) - (2 * q.z * q.w);
    m.m[1][1] = 1 - (2 * q.x * q.x) - (2 * (q.z *  q.z));
    m.m[1][2] = 2 * (q.y * q.z) + 2 * (q.x * q.w);
    m.m[1][3] = 0;

    m.m[2][0] = (2 * q.x * q.z) + (2 * q.y * q.w);
    m.m[2][1] = (2 * q.y  * q.z) - (2 * q.x * q.w);
    m.m[2][2] = 1 - (2 * q.x * q.x) - (2 * (q.y *  q.y));
    m.m[2][3] = 0;

    m.m[3][0] = 0;
    m.m[3][1] = 0;
    m.m[3][2] = 0;
    m.m[3][3] = 1;

    return m;
}

mat4 genIdentityMatrix(){
    mat4 m = {  
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
        };
    return m;
}

mat4 setPerspectiveProjection(float fov, float aspect, float znear, float zfar){
        mat4 projection;
        float tanHalfFov = tan(fov / 2);
        projection.m[0][0] = 1.0f / (aspect * tanHalfFov); 
        projection.m[1][0] = 0; 
        projection.m[2][0] = 0; 
        projection.m[3][0] = 0;

        projection.m[0][1] = 0;
        projection.m[1][1] = 1.0f / tanHalfFov; 
        projection.m[2][1] = 0; 
        projection.m[3][1] = 0;

        projection.m[0][2] = 0;
        projection.m[1][2] = 0;
        projection.m[2][2] = -(zfar + znear) / (zfar - znear);
        projection.m[3][2] = -(2 * zfar * znear) / (zfar - znear);

        projection.m[0][3] = 0;
        projection.m[1][3] = 0;
        projection.m[2][3] = -1;
        projection.m[3][3] = 0;

        return projection;
    }
    
    mat4 setOrthogonalProjection(float left, float right, float bottom, float top, float near, float far){
        mat4 projection;
        projection.m[0][0] = 2.0f / (right - left); 
        projection.m[1][0] = 0; 
        projection.m[2][0] = 0; 
        projection.m[3][0] = -((right + left) / (right - left));

        projection.m[0][1] = 0;
        projection.m[1][1] = 2.0f / (top - bottom); 
        projection.m[2][1] = 0; 
        projection.m[3][1] = -((top + bottom) / (top - bottom));

        projection.m[0][2] = 0;
        projection.m[1][2] = 0;
        projection.m[2][2] = -2.0f / (far - near);
        projection.m[3][2] = -((far + near) / (far - near));

        projection.m[0][3] = 0;
        projection.m[1][3] = 0;
        projection.m[2][3] = 0;
        projection.m[3][3] = 1.0f;

        return projection;
    }