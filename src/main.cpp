#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "../include/json.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

using json = nlohmann::json;

struct Artwork {
    int id = 0;
    std::string title;
    std::string category;
    std::string description;
    std::string image;
    int width = 0;
    int height = 0;
    std::string source;
    std::string license;

    GLuint textureID = 0;
    float x = 0.0f;
    float y = 2.8f;
    float z = -9.99f;
    float drawWidth = 2.5f;
    float drawHeight = 2.0f;
};

// Camera
float camX = 0.0f;
float camY = 2.0f;
float camZ = 8.0f;
float camYaw = 0.0f;

// Gallery
std::vector<Artwork> gallery;

std::vector<Artwork> loadGallery(const std::string& path, int maxItems = 5) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + path);
    }

    json j;
    file >> j;

    std::vector<Artwork> result;
    int count = 0;

    for (const auto& item : j) {
        if (count >= maxItems) break;

        Artwork art;
        art.id = item.value("id", 0);
        art.title = item.value("title", "Untitled");
        art.category = item.value("category", "General");
        art.description = item.value("description", "");
        art.image = item.value("image", "");
        art.width = item.value("width", 0);
        art.height = item.value("height", 0);
        art.source = item.value("source", "Unknown");
        art.license = item.value("license", "public_domain");

        result.push_back(art);
        count++;
    }

    return result;
}

GLuint loadTexture(const char* path) {
    int width, height, channels;

    std::cout << "Loading image: " << path << "\n";
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);

    if (!data) {
        std::cerr << "Failed to load image: " << path << "\n";
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 1) format = GL_LUMINANCE;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    std::cout << "Loaded texture OK: " << width << "x" << height << "\n";
    return textureID;
}

void prepareArtworks() {
    const float positions[5] = { -6.0f, -3.0f, 0.0f, 3.0f, 6.0f };

    for (size_t i = 0; i < gallery.size(); i++) {
        Artwork& art = gallery[i];

        float aspect = 1.0f;
        if (art.height > 0) {
            aspect = static_cast<float>(art.width) / static_cast<float>(art.height);
        }

        float h = 2.2f;
        float w = h * aspect;

        if (w > 2.4f) {
            w = 2.4f;
            h = w / aspect;
        }

        if (h > 2.8f) {
            h = 2.8f;
            w = h * aspect;
        }

        art.drawWidth = w;
        art.drawHeight = h;
        art.x = positions[i];
        art.y = 2.8f;
        art.z = -9.95f;

        art.textureID = loadTexture(art.image.c_str());
    }
}

void drawQuad(float x1, float y1, float z1,
              float x2, float y2, float z2,
              float x3, float y3, float z3,
              float x4, float y4, float z4) {
    glBegin(GL_QUADS);
        glVertex3f(x1, y1, z1);
        glVertex3f(x2, y2, z2);
        glVertex3f(x3, y3, z3);
        glVertex3f(x4, y4, z4);
    glEnd();
}

void drawPedestal(float cx, float cy, float cz, float w, float h, float d) {
    float x1 = cx - w / 2.0f;
    float x2 = cx + w / 2.0f;
    float y1 = cy;
    float y2 = cy + h;
    float z1 = cz - d / 2.0f;
    float z2 = cz + d / 2.0f;

    glColor3f(0.82f, 0.82f, 0.80f);

    // front
    drawQuad(x1, y1, z2, x2, y1, z2, x2, y2, z2, x1, y2, z2);
    // back
    drawQuad(x2, y1, z1, x1, y1, z1, x1, y2, z1, x2, y2, z1);
    // left
    drawQuad(x1, y1, z1, x1, y1, z2, x1, y2, z2, x1, y2, z1);
    // right
    drawQuad(x2, y1, z2, x2, y1, z1, x2, y2, z1, x2, y2, z2);
    // top
    drawQuad(x1, y2, z1, x2, y2, z1, x2, y2, z2, x1, y2, z2);
}

void drawRoom() {
    // Floor
    glColor3f(0.68f, 0.68f, 0.66f);
    drawQuad(-10.0f, 0.0f, -10.0f, 10.0f, 0.0f, -10.0f, 10.0f, 0.0f, 10.0f, -10.0f, 0.0f, 10.0f);

    // Ceiling
    glColor3f(0.95f, 0.95f, 0.93f);
    drawQuad(-10.0f, 6.0f, 10.0f, 10.0f, 6.0f, 10.0f, 10.0f, 6.0f, -10.0f, -10.0f, 6.0f, -10.0f);

    // Back wall
    glColor3f(0.90f, 0.89f, 0.84f);
    drawQuad(-10.0f, 0.0f, -10.0f, 10.0f, 0.0f, -10.0f, 10.0f, 6.0f, -10.0f, -10.0f, 6.0f, -10.0f);

    // Left wall
    glColor3f(0.91f, 0.90f, 0.86f);
    drawQuad(-10.0f, 0.0f, -10.0f, -10.0f, 0.0f, 10.0f, -10.0f, 6.0f, 10.0f, -10.0f, 6.0f, -10.0f);

    // Right wall
    glColor3f(0.91f, 0.90f, 0.86f);
    drawQuad(10.0f, 0.0f, 10.0f, 10.0f, 0.0f, -10.0f, 10.0f, 6.0f, -10.0f, 10.0f, 6.0f, 10.0f);

    // Front wall, split into two sections with middle opening
    glColor3f(0.89f, 0.88f, 0.84f);
    // left section
    drawQuad(-10.0f, 0.0f, 10.0f, -3.0f, 0.0f, 10.0f, -3.0f, 6.0f, 10.0f, -10.0f, 6.0f, 10.0f);
    // right section
    drawQuad(3.0f, 0.0f, 10.0f, 10.0f, 0.0f, 10.0f, 10.0f, 6.0f, 10.0f, 3.0f, 6.0f, 10.0f);
    // top section above doorway
    drawQuad(-3.0f, 4.2f, 10.0f, 3.0f, 4.2f, 10.0f, 3.0f, 6.0f, 10.0f, -3.0f, 6.0f, 10.0f);

    // Baseboards / trim near floor
    glColor3f(0.55f, 0.45f, 0.35f);
    // back trim
    drawQuad(-10.0f, 0.0f, -9.98f, 10.0f, 0.0f, -9.98f, 10.0f, 0.25f, -9.98f, -10.0f, 0.25f, -9.98f);
    // left trim
    drawQuad(-9.98f, 0.0f, -10.0f, -9.98f, 0.0f, 10.0f, -9.98f, 0.25f, 10.0f, -9.98f, 0.25f, -10.0f);
    // right trim
    drawQuad(9.98f, 0.0f, 10.0f, 9.98f, 0.0f, -10.0f, 9.98f, 0.25f, -10.0f, 9.98f, 0.25f, 10.0f);

    // Center pedestal
    drawPedestal(0.0f, 0.0f, -1.5f, 2.0f, 1.2f, 2.0f);
}

void drawFrame(float centerX, float centerY, float z, float w, float h) {
    float x1 = centerX - w / 2.0f;
    float x2 = centerX + w / 2.0f;
    float y1 = centerY - h / 2.0f;
    float y2 = centerY + h / 2.0f;

    glColor3f(0.35f, 0.20f, 0.10f);
    glLineWidth(4.0f);
    glBegin(GL_LINE_LOOP);
        glVertex3f(x1, y1, z + 0.002f);
        glVertex3f(x2, y1, z + 0.002f);
        glVertex3f(x2, y2, z + 0.002f);
        glVertex3f(x1, y2, z + 0.002f);
    glEnd();
}

void drawPaintings() {
    glEnable(GL_TEXTURE_2D);

    for (const auto& art : gallery) {
        if (art.textureID == 0) continue;

        float x1 = art.x - art.drawWidth / 2.0f;
        float x2 = art.x + art.drawWidth / 2.0f;
        float y1 = art.y - art.drawHeight / 2.0f;
        float y2 = art.y + art.drawHeight / 2.0f;

        glBindTexture(GL_TEXTURE_2D, art.textureID);
        glColor3f(1.0f, 1.0f, 1.0f);

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(x1, y1, art.z);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(x2, y1, art.z);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(x2, y2, art.z);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(x1, y2, art.z);
        glEnd();

        glBindTexture(GL_TEXTURE_2D, 0);
        drawFrame(art.x, art.y, art.z, art.drawWidth, art.drawHeight);
    }

    glDisable(GL_TEXTURE_2D);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float lookX = camX + std::sin(camYaw);
    float lookZ = camZ - std::cos(camYaw);

    gluLookAt(camX, camY, camZ, lookX, camY, lookZ, 0.0f, 1.0f, 0.0f);

    drawRoom();
    drawPaintings();

    glutSwapBuffers();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, static_cast<float>(w) / static_cast<float>(h), 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void clampCamera() {
    if (camX < -8.7f) camX = -8.7f;
    if (camX >  8.7f) camX =  8.7f;
    if (camZ < -8.0f) camZ = -8.0f;
    if (camZ >  8.7f) camZ =  8.7f;

    // avoid pedestal area roughly
    if (camX > -1.5f && camX < 1.5f && camZ > -3.0f && camZ < 0.3f) {
        if (camZ < -1.35f) camZ = -3.0f;
        else camZ = 0.3f;
    }
}

void keyboard(unsigned char key, int, int) {
    float move = 0.35f;
    float turn = 0.08f;

    switch (key) {
        case 'w': camX += std::sin(camYaw) * move; camZ -= std::cos(camYaw) * move; break;
        case 's': camX -= std::sin(camYaw) * move; camZ += std::cos(camYaw) * move; break;
        case 'a': camX -= std::cos(camYaw) * move; camZ -= std::sin(camYaw) * move; break;
        case 'd': camX += std::cos(camYaw) * move; camZ += std::sin(camYaw) * move; break;
        case 'j': camYaw -= turn; break;
        case 'l': camYaw += turn; break;
        case 27: std::exit(0);
    }

    clampCamera();
    glutPostRedisplay();
}

void init() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.15f, 0.15f, 0.18f, 1.0f);

    gallery = loadGallery("data/gallery.json", 5);
    prepareArtworks();

    std::cout << "Prepared " << gallery.size() << " paintings.\n";
}

int main(int argc, char** argv) {
    try {
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
        glutInitWindowSize(1000, 700);
        glutCreateWindow("3D Art Gallery");

        init();

        glutDisplayFunc(display);
        glutReshapeFunc(reshape);
        glutKeyboardFunc(keyboard);

        glutMainLoop();
    } catch (const std::exception& e) {
        std::cerr << "Startup error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
