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
    float z = -9.95f;
    float drawWidth = 2.5f;
    float drawHeight = 2.0f;

    int room = 1;
};

// Camera
float camX = 0.0f;
float camY = 2.0f;
float camZ = 6.5f;
float camYaw = 0.0f;

// Gallery
std::vector<Artwork> gallery;

// Room layout
const float ROOM1_CENTER_X = 0.0f;
const float ROOM2_CENTER_X = 24.0f;
const float ROOM_HALF_W    = 10.0f;
const float ROOM_HALF_D    = 10.0f;
const float ROOM_H         = 6.0f;

// Door / corridor layout
const float DOOR_Z1 = -4.0f;
const float DOOR_Z2 =  4.0f;
const float DOOR_TOP = 4.4f;

std::vector<Artwork> loadGallery(const std::string& path, int maxItems = 10) {
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
    const float room1Positions[5] = { -6.0f, -3.0f, 0.0f, 3.0f, 6.0f };
    const float room2Positions[5] = { -6.0f, -3.0f, 0.0f, 3.0f, 6.0f };

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
        art.y = 2.8f;

        if (i < 5) {
            art.room = 1;
            art.x = ROOM1_CENTER_X + room1Positions[i];
            art.z = -9.95f;
        } else {
            art.room = 2;
            art.x = ROOM2_CENTER_X + room2Positions[i - 5];
            art.z = -9.95f;
        }

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

    drawQuad(x1, y1, z2, x2, y1, z2, x2, y2, z2, x1, y2, z2);
    drawQuad(x2, y1, z1, x1, y1, z1, x1, y2, z1, x2, y2, z1);
    drawQuad(x1, y1, z1, x1, y1, z2, x1, y2, z2, x1, y2, z1);
    drawQuad(x2, y1, z2, x2, y1, z1, x2, y2, z1, x2, y2, z2);
    drawQuad(x1, y2, z1, x2, y2, z1, x2, y2, z2, x1, y2, z2);
}

void drawFrontWallWithOpening(float cx) {
    float left   = cx - ROOM_HALF_W;
    float right  = cx + ROOM_HALF_W;
    float frontZ = ROOM_HALF_D;

    glColor3f(0.89f, 0.88f, 0.84f);
    drawQuad(left, 0.0f, frontZ, cx - 3.0f, 0.0f, frontZ, cx - 3.0f, ROOM_H, frontZ, left, ROOM_H, frontZ);
    drawQuad(cx + 3.0f, 0.0f, frontZ, right, 0.0f, frontZ, right, ROOM_H, frontZ, cx + 3.0f, ROOM_H, frontZ);
    drawQuad(cx - 3.0f, 4.2f, frontZ, cx + 3.0f, 4.2f, frontZ, cx + 3.0f, ROOM_H, frontZ, cx - 3.0f, ROOM_H, frontZ);
}

void drawLeftWallSolid(float cx) {
    float left = cx - ROOM_HALF_W;
    drawQuad(left, 0.0f, -ROOM_HALF_D, left, 0.0f, ROOM_HALF_D, left, ROOM_H, ROOM_HALF_D, left, ROOM_H, -ROOM_HALF_D);
}

void drawRightWallSolid(float cx) {
    float right = cx + ROOM_HALF_W;
    drawQuad(right, 0.0f, ROOM_HALF_D, right, 0.0f, -ROOM_HALF_D, right, ROOM_H, -ROOM_HALF_D, right, ROOM_H, ROOM_HALF_D);
}

void drawRightWallWithDoor(float cx) {
    float x = cx + ROOM_HALF_W;

    glColor3f(0.91f, 0.90f, 0.86f);
    // lower front section
    drawQuad(x, 0.0f, ROOM_HALF_D, x, 0.0f, DOOR_Z2, x, ROOM_H, DOOR_Z2, x, ROOM_H, ROOM_HALF_D);
    // lower back section
    drawQuad(x, 0.0f, DOOR_Z1, x, 0.0f, -ROOM_HALF_D, x, ROOM_H, -ROOM_HALF_D, x, ROOM_H, DOOR_Z1);
    // top above doorway
    drawQuad(x, DOOR_TOP, DOOR_Z1, x, DOOR_TOP, DOOR_Z2, x, ROOM_H, DOOR_Z2, x, ROOM_H, DOOR_Z1);
}

void drawLeftWallWithDoor(float cx) {
    float x = cx - ROOM_HALF_W;

    glColor3f(0.91f, 0.90f, 0.86f);
    // lower back section
    drawQuad(x, 0.0f, -ROOM_HALF_D, x, 0.0f, DOOR_Z1, x, ROOM_H, DOOR_Z1, x, ROOM_H, -ROOM_HALF_D);
    // lower front section
    drawQuad(x, 0.0f, DOOR_Z2, x, 0.0f, ROOM_HALF_D, x, ROOM_H, ROOM_HALF_D, x, ROOM_H, DOOR_Z2);
    // top above doorway
    drawQuad(x, DOOR_TOP, DOOR_Z2, x, DOOR_TOP, DOOR_Z1, x, ROOM_H, DOOR_Z1, x, ROOM_H, DOOR_Z2);
}

void drawBaseTrim(float cx) {
    float left   = cx - ROOM_HALF_W;
    float right  = cx + ROOM_HALF_W;

    glColor3f(0.55f, 0.45f, 0.35f);
    drawQuad(left, 0.0f, -ROOM_HALF_D + 0.02f, right, 0.0f, -ROOM_HALF_D + 0.02f, right, 0.25f, -ROOM_HALF_D + 0.02f, left, 0.25f, -ROOM_HALF_D + 0.02f);
    drawQuad(left + 0.02f, 0.0f, -ROOM_HALF_D, left + 0.02f, 0.0f, ROOM_HALF_D, left + 0.02f, 0.25f, ROOM_HALF_D, left + 0.02f, 0.25f, -ROOM_HALF_D);
    drawQuad(right - 0.02f, 0.0f, ROOM_HALF_D, right - 0.02f, 0.0f, -ROOM_HALF_D, right - 0.02f, 0.25f, -ROOM_HALF_D, right - 0.02f, 0.25f, ROOM_HALF_D);
}

void drawRoomShellRoom1() {
    float left   = ROOM1_CENTER_X - ROOM_HALF_W;
    float right  = ROOM1_CENTER_X + ROOM_HALF_W;

    // Floor
    glColor3f(0.68f, 0.68f, 0.66f);
    drawQuad(left, 0.0f, -ROOM_HALF_D, right, 0.0f, -ROOM_HALF_D, right, 0.0f, ROOM_HALF_D, left, 0.0f, ROOM_HALF_D);

    // Ceiling
    glColor3f(0.95f, 0.95f, 0.93f);
    drawQuad(left, ROOM_H, ROOM_HALF_D, right, ROOM_H, ROOM_HALF_D, right, ROOM_H, -ROOM_HALF_D, left, ROOM_H, -ROOM_HALF_D);

    // Back wall
    glColor3f(0.90f, 0.89f, 0.84f);
    drawQuad(left, 0.0f, -ROOM_HALF_D, right, 0.0f, -ROOM_HALF_D, right, ROOM_H, -ROOM_HALF_D, left, ROOM_H, -ROOM_HALF_D);

    // Left wall solid
    glColor3f(0.91f, 0.90f, 0.86f);
    drawLeftWallSolid(ROOM1_CENTER_X);

    // Right wall with side doorway
    drawRightWallWithDoor(ROOM1_CENTER_X);

    // Front wall with opening
    drawFrontWallWithOpening(ROOM1_CENTER_X);

    // Trim
    drawBaseTrim(ROOM1_CENTER_X);
}

void drawRoomShellRoom2() {
    float left   = ROOM2_CENTER_X - ROOM_HALF_W;
    float right  = ROOM2_CENTER_X + ROOM_HALF_W;

    // Floor
    glColor3f(0.68f, 0.68f, 0.66f);
    drawQuad(left, 0.0f, -ROOM_HALF_D, right, 0.0f, -ROOM_HALF_D, right, 0.0f, ROOM_HALF_D, left, 0.0f, ROOM_HALF_D);

    // Ceiling
    glColor3f(0.95f, 0.95f, 0.93f);
    drawQuad(left, ROOM_H, ROOM_HALF_D, right, ROOM_H, ROOM_HALF_D, right, ROOM_H, -ROOM_HALF_D, left, ROOM_H, -ROOM_HALF_D);

    // Back wall
    glColor3f(0.90f, 0.89f, 0.84f);
    drawQuad(left, 0.0f, -ROOM_HALF_D, right, 0.0f, -ROOM_HALF_D, right, ROOM_H, -ROOM_HALF_D, left, ROOM_H, -ROOM_HALF_D);

    // Left wall with side doorway
    drawLeftWallWithDoor(ROOM2_CENTER_X);

    // Right wall solid
    glColor3f(0.91f, 0.90f, 0.86f);
    drawRightWallSolid(ROOM2_CENTER_X);

    // Front wall with opening
    drawFrontWallWithOpening(ROOM2_CENTER_X);

    // Trim
    drawBaseTrim(ROOM2_CENTER_X);
}

void drawConnectingCorridor() {
    float leftX = ROOM1_CENTER_X + ROOM_HALF_W;
    float rightX = ROOM2_CENTER_X - ROOM_HALF_W;
    float z1 = DOOR_Z1;
    float z2 = DOOR_Z2;

    // floor
    glColor3f(0.66f, 0.66f, 0.64f);
    drawQuad(leftX, 0.0f, z1, rightX, 0.0f, z1, rightX, 0.0f, z2, leftX, 0.0f, z2);

    // ceiling
    glColor3f(0.94f, 0.94f, 0.92f);
    drawQuad(leftX, ROOM_H, z2, rightX, ROOM_H, z2, rightX, ROOM_H, z1, leftX, ROOM_H, z1);

    // corridor front side wall
    glColor3f(0.90f, 0.89f, 0.85f);
    drawQuad(leftX, 0.0f, z2, rightX, 0.0f, z2, rightX, ROOM_H, z2, leftX, ROOM_H, z2);

    // corridor back side wall
    drawQuad(rightX, 0.0f, z1, leftX, 0.0f, z1, leftX, ROOM_H, z1, rightX, ROOM_H, z1);
}

void drawRoom() {
    drawRoomShellRoom1();
    drawRoomShellRoom2();
    drawConnectingCorridor();

    drawPedestal(ROOM1_CENTER_X, 0.0f, -1.5f, 2.0f, 1.2f, 2.0f);
    drawPedestal(ROOM2_CENTER_X, 0.0f, -1.5f, 2.0f, 1.2f, 2.0f);
}

void drawFrameBack(float centerX, float centerY, float z, float w, float h) {
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
        drawFrameBack(art.x, art.y, art.z, art.drawWidth, art.drawHeight);
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
    gluPerspective(65.0f, static_cast<float>(w) / static_cast<float>(h), 0.1f, 150.0f);
    glMatrixMode(GL_MODELVIEW);
}

void clampCamera() {
    if (camX < -8.7f) camX = -8.7f;
    if (camX > 32.7f) camX = 32.7f;
    if (camZ < -8.0f) camZ = -8.0f;
    if (camZ > 8.7f) camZ = 8.7f;

    // pedestal room 1
    if (camX > -1.5f && camX < 1.5f && camZ > -3.0f && camZ < 0.3f) {
        if (camZ < -1.35f) camZ = -3.0f;
        else camZ = 0.3f;
    }

    // pedestal room 2
    if (camX > 22.5f && camX < 25.5f && camZ > -3.0f && camZ < 0.3f) {
        if (camZ < -1.35f) camZ = -3.0f;
        else camZ = 0.3f;
    }
}

void keyboard(unsigned char key, int, int) {
    float move = 0.50f;
    float turn = 0.10f;

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

    gallery = loadGallery("data/gallery.json", 10);
    prepareArtworks();

    std::cout << "Prepared " << gallery.size() << " paintings.\n";
    std::cout << "Walk to the right side of Room 1 to enter the corridor.\n";
}

int main(int argc, char** argv) {
    try {
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
        glutInitWindowSize(1280, 800);
        glutCreateWindow("3D Art Gallery - Two Connected Rooms");

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
