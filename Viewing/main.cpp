#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "maths_funcs.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

#define MESH_NAME SHADER_DIR "/monkeyhead_smooth.dae"

struct ModelData {
    size_t mPointCount = 0;
    std::vector<vec3> mVertices;
    std::vector<vec3> mNormals;
    std::vector<vec2> mTextureCoords;
};

GLuint shaderProgramID;
ModelData mesh_data;
GLuint vao = 0;
GLuint loc1, loc2;
GLfloat rotate_y = 0.0f;
int width = 800, height = 600;
bool topView = false;
bool usePerspective = true;
float fov = 45.0f;
float cameraDistance = 10.0f;
float farPlane = 100.0f;

// --------------------------------------------------
// Mesh loading (Assimp)
// --------------------------------------------------
ModelData load_mesh(const char* file_name) {
    ModelData modelData;
    const aiScene* scene = aiImportFile(
            file_name,
            aiProcess_Triangulate | aiProcess_PreTransformVertices
    );

    if (!scene) {
        fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
        return modelData;
    }

    printf("Loaded: %i meshes\n", scene->mNumMeshes);

    for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
        const aiMesh* mesh = scene->mMeshes[m_i];
        modelData.mPointCount += mesh->mNumVertices;
        for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
            if (mesh->HasPositions()) {
                const aiVector3D* vp = &(mesh->mVertices[v_i]);
                modelData.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
            }
            if (mesh->HasNormals()) {
                const aiVector3D* vn = &(mesh->mNormals[v_i]);
                modelData.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
            }
            if (mesh->HasTextureCoords(0)) {
                const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
                modelData.mTextureCoords.push_back(vec2(vt->x, vt->y));
            }
        }
    }

    aiReleaseImport(scene);
    return modelData;
}

// --------------------------------------------------
// Shader helpers
// --------------------------------------------------

char* readShaderSource(const char* shaderFile) {
    FILE* fp = fopen(shaderFile, "rb");
    if (!fp) {
        fprintf(stderr, "❌ ERROR: Cannot open shader file: %s\n", shaderFile);
        return nullptr;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    if (size <= 0) {
        fprintf(stderr, "❌ ERROR: Shader file %s is empty or unreadable\n", shaderFile);
        fclose(fp);
        return nullptr;
    }
    fseek(fp, 0, SEEK_SET);

    char* buf = new char[size + 1];
    size_t readCount = fread(buf, 1, size, fp);
    fclose(fp);

    if (readCount != (size_t)size) {
        fprintf(stderr, "❌ ERROR: Failed to read shader file completely: %s\n", shaderFile);
        delete[] buf;
        return nullptr;
    }

    buf[size] = '\0';
    return buf;
}


static void AddShader(GLuint program, const char* file, GLenum type) {
    char* src = readShaderSource(file);
    if (!src) {
        std::cerr << "❌ ERROR: Shader source is null for " << file << std::endl;
        exit(EXIT_FAILURE);
    }
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint success;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar log[1024];
        glGetShaderInfoLog(sh, 1024, nullptr, log);
        std::cerr << "Shader compile error (" << file << "): " << log << std::endl;
        exit(1);
    }
    glAttachShader(program, sh);
    delete[] src;
}

GLuint CompileShaders() {
    GLuint program = glCreateProgram();
    AddShader(program, SHADER_DIR "/simpleVertexShader.txt", GL_VERTEX_SHADER);
    AddShader(program, SHADER_DIR "/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "Link error: " << log << std::endl;
        exit(1);
    }

    glUseProgram(program);
    return program;
}

// --------------------------------------------------
// Buffer setup
// --------------------------------------------------
void generateObjectBufferMesh() {
    mesh_data = load_mesh(MESH_NAME);

    GLuint vp_vbo, vn_vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vp_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh_data.mVertices.size() * sizeof(vec3), mesh_data.mVertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &vn_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh_data.mNormals.size() * sizeof(vec3), mesh_data.mNormals.data(), GL_STATIC_DRAW);

    loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
    loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");

    glEnableVertexAttribArray(loc1);
    glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
    glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glEnableVertexAttribArray(loc2);
    glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
    glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindVertexArray(0);
}

// --------------------------------------------------
// Render
// --------------------------------------------------
void drawScene() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);
    glBindVertexArray(vao);

    int model_loc = glGetUniformLocation(shaderProgramID, "model");
    int view_loc  = glGetUniformLocation(shaderProgramID, "view");
    int proj_loc  = glGetUniformLocation(shaderProgramID, "proj");

    // View Matrix
    glm::mat4 view;

    if (!topView) {
        // Front View
        view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, cameraDistance),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }
    else {
        // Top View
        view = glm::lookAt(
            glm::vec3(0.0f, 10.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, -1.0f)
        );
    }
    // Projection Matrix
    float aspect = (float)width / height;

    glm::mat4 proj;

    if (usePerspective) {
        proj = glm::perspective(
            glm::radians(fov),
            aspect,
            0.1f,
            farPlane
        );
    }
    else {
        float halfHeight = 3.0f;

        proj = glm::ortho(
            -halfHeight * aspect,
            halfHeight * aspect,
            -halfHeight,
            halfHeight,
            0.1f,
            farPlane
        );
    }

    glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(proj));

    // Object 1 - closer to camera
    glm::mat4 model1 = glm::mat4(1.0f);
    model1 = glm::translate(
        model1,
        glm::vec3(-1.5f, 0.0f, 1.0f)
    );

    glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model1));
    glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

    // Object 2 - farther from camera
    glm::mat4 model2 = glm::mat4(1.0f);
    model2 = glm::translate(
        model2,
        glm::vec3(1.5f, 0.0f, -1.0f)
    );

    glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model2));
    glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

    glBindVertexArray(0);
}

void keyCallback(
    GLFWwindow* window,
    int key,
    int scancode,
    int action,
    int mods
) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;

    switch (key) {
        // Camera view
        case GLFW_KEY_1:
            topView = false;
            break;

        case GLFW_KEY_2:
            topView = true;
            break;

        // Projection type
        case GLFW_KEY_P:
            usePerspective = true;
            break;

        case GLFW_KEY_O:
            usePerspective = false;
            break;

        // Field of view
        case GLFW_KEY_3:
            fov = 30.0f;
            break;

        case GLFW_KEY_4:
            fov = 45.0f;
            break;

        case GLFW_KEY_5:
            fov = 75.0f;
            break;

        // Camera distance
        case GLFW_KEY_UP:
            cameraDistance += 1.0f;
            break;

        case GLFW_KEY_DOWN:
            cameraDistance = std::max(2.0f, cameraDistance - 1.0f);
            break;

        // Far clipping plane
        case GLFW_KEY_LEFT_BRACKET:
            farPlane = std::max(0.2f, farPlane - 1.0f);
            break;

        case GLFW_KEY_RIGHT_BRACKET:
            farPlane += 1.0f;
            break;

        // Reset
        case GLFW_KEY_R:
            topView = false;
            usePerspective = true;
            fov = 45.0f;
            cameraDistance = 10.0f;
            farPlane = 100.0f;
            break;
    }
}

// --------------------------------------------------
// Main (GLFW loop)
// --------------------------------------------------
int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(width, height, "Lab 4", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);

    shaderProgramID = CompileShaders();
    generateObjectBufferMesh();

    float lastTime = (float)glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float delta = now - lastTime;
        lastTime = now;

        drawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
