#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
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

// Previous camera position for collision rollback
float prevCamX = 0.0f;
float prevCamZ = 6.5f;

// Window size
int windowWidth = 1200;
int windowHeight = 800;

// Input state
bool keyStates[256] = {false};

// Gallery
std::vector<Artwork> gallery;

// Interaction
int nearestArtworkIndex = -1;
bool showInfoPanel = false;
bool showMiniMap = true;
bool showHUD = true;

// Room layout
const float ROOM1_CENTER_X = 0.0f;
const float ROOM2_CENTER_X = 24.0f;
const float ROOM3_CENTER_X = 48.0f;

const float ROOM_HALF_W    = 10.0f;
const float ROOM_HALF_D    = 10.0f;
const float ROOM_H         = 6.0f;

// Door / corridor layout
const float DOOR_Z1 = -4.0f;
const float DOOR_Z2 =  4.0f;
const float DOOR_TOP = 4.4f;

// -------------------- Data --------------------

std::vector<Artwork> loadGallery(const std::string& path, int maxItems = 15) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + path);
    }

    json j;
    file >> j;

    std::vector<Artwork> result;

    for (const auto& item : j) {
        if ((int)result.size() >= maxItems) break;

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

        std::ifstream imgTest(art.image);
        if (!imgTest.good()) {
            std::cout << "Skipping missing image: " << art.image << "\n";
            continue;
        }

        result.push_back(art);
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
        art.y = 2.8f;

        if (i < 5) {
            art.room = 1;
            art.x = ROOM1_CENTER_X + positions[i];
        } else if (i < 10) {
            art.room = 2;
            art.x = ROOM2_CENTER_X + positions[i - 5];
        } else {
            art.room = 3;
            art.x = ROOM3_CENTER_X + positions[i - 10];
        }

        art.z = -9.95f;
        art.textureID = loadTexture(art.image.c_str());
    }
}

// -------------------- Drawing helpers --------------------

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

void drawText3D(float x, float y, float z, const std::string& text, float scale = 0.0018f) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);
    for (char c : text) {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
    }
    glPopMatrix();
}

void drawBitmapText2D(float x, float y, const std::string& text) {
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
}

std::string truncateTitle(const std::string& s, size_t maxLen = 18) {
    if (s.size() <= maxLen) return s;
    return s.substr(0, maxLen - 3) + "...";
}

std::string truncateLine(const std::string& s, size_t maxLen = 42) {
    if (s.empty()) return "(no description)";
    if (s.size() <= maxLen) return s;
    return s.substr(0, maxLen - 3) + "...";
}

std::string currentRoomName() {
    if (camX < 10.0f) return "Landscape Hall";
    if (camX < 34.0f) {
        if (camX <= 14.0f) return "Corridor 1";
        return "Architecture Hall";
    }
    if (camX <= 38.0f) return "Corridor 2";
    return "Space Hall";
}

// -------------------- Room geometry --------------------

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
    drawQuad(x, 0.0f, ROOM_HALF_D, x, 0.0f, DOOR_Z2, x, ROOM_H, DOOR_Z2, x, ROOM_H, ROOM_HALF_D);
    drawQuad(x, 0.0f, DOOR_Z1, x, 0.0f, -ROOM_HALF_D, x, ROOM_H, -ROOM_HALF_D, x, ROOM_H, DOOR_Z1);
    drawQuad(x, DOOR_TOP, DOOR_Z1, x, DOOR_TOP, DOOR_Z2, x, ROOM_H, DOOR_Z2, x, ROOM_H, DOOR_Z1);
}

void drawLeftWallWithDoor(float cx) {
    float x = cx - ROOM_HALF_W;

    glColor3f(0.91f, 0.90f, 0.86f);
    drawQuad(x, 0.0f, -ROOM_HALF_D, x, 0.0f, DOOR_Z1, x, ROOM_H, DOOR_Z1, x, ROOM_H, -ROOM_HALF_D);
    drawQuad(x, 0.0f, DOOR_Z2, x, 0.0f, ROOM_HALF_D, x, ROOM_H, ROOM_HALF_D, x, ROOM_H, DOOR_Z2);
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

void drawRoomShell(float cx, bool doorLeft, bool doorRight) {
    float left   = cx - ROOM_HALF_W;
    float right  = cx + ROOM_HALF_W;

    glColor3f(0.68f, 0.68f, 0.66f);
    drawQuad(left, 0.0f, -ROOM_HALF_D, right, 0.0f, -ROOM_HALF_D, right, 0.0f, ROOM_HALF_D, left, 0.0f, ROOM_HALF_D);

    glColor3f(0.95f, 0.95f, 0.93f);
    drawQuad(left, ROOM_H, ROOM_HALF_D, right, ROOM_H, ROOM_HALF_D, right, ROOM_H, -ROOM_HALF_D, left, ROOM_H, -ROOM_HALF_D);

    glColor3f(0.90f, 0.89f, 0.84f);
    drawQuad(left, 0.0f, -ROOM_HALF_D, right, 0.0f, -ROOM_HALF_D, right, ROOM_H, -ROOM_HALF_D, left, ROOM_H, -ROOM_HALF_D);

    if (doorLeft) drawLeftWallWithDoor(cx);
    else {
        glColor3f(0.91f, 0.90f, 0.86f);
        drawLeftWallSolid(cx);
    }

    if (doorRight) drawRightWallWithDoor(cx);
    else {
        glColor3f(0.91f, 0.90f, 0.86f);
        drawRightWallSolid(cx);
    }

    drawFrontWallWithOpening(cx);
    drawBaseTrim(cx);
}

void drawConnectingCorridor(float leftX, float rightX) {
    float z1 = DOOR_Z1;
    float z2 = DOOR_Z2;

    glColor3f(0.66f, 0.66f, 0.64f);
    drawQuad(leftX, 0.0f, z1, rightX, 0.0f, z1, rightX, 0.0f, z2, leftX, 0.0f, z2);

    glColor3f(0.94f, 0.94f, 0.92f);
    drawQuad(leftX, ROOM_H, z2, rightX, ROOM_H, z2, rightX, ROOM_H, z1, leftX, ROOM_H, z1);

    glColor3f(0.90f, 0.89f, 0.85f);
    drawQuad(leftX, 0.0f, z2, rightX, 0.0f, z2, rightX, ROOM_H, z2, leftX, ROOM_H, z2);
    drawQuad(rightX, 0.0f, z1, leftX, 0.0f, z1, leftX, ROOM_H, z1, rightX, ROOM_H, z1);
}

void drawRoom() {
    drawRoomShell(ROOM1_CENTER_X, false, true);
    drawRoomShell(ROOM2_CENTER_X, true, true);
    drawRoomShell(ROOM3_CENTER_X, true, false);

    drawConnectingCorridor(ROOM1_CENTER_X + ROOM_HALF_W, ROOM2_CENTER_X - ROOM_HALF_W);
    drawConnectingCorridor(ROOM2_CENTER_X + ROOM_HALF_W, ROOM3_CENTER_X - ROOM_HALF_W);

    drawPedestal(ROOM1_CENTER_X, 0.0f, -1.5f, 2.0f, 1.2f, 2.0f);
    drawPedestal(ROOM2_CENTER_X, 0.0f, -1.5f, 2.0f, 1.2f, 2.0f);
    drawPedestal(ROOM3_CENTER_X, 0.0f, -1.5f, 2.0f, 1.2f, 2.0f);
}

// -------------------- Extra polish --------------------

void drawPaintingSpotlights() {
    glDisable(GL_TEXTURE_2D);

    for (const auto& art : gallery) {
        glColor3f(0.95f, 0.92f, 0.70f);
        float lx1 = art.x - 0.35f;
        float lx2 = art.x + 0.35f;
        float ly1 = art.y + art.drawHeight / 2.0f + 0.25f;
        float ly2 = ly1 + 0.10f;
        float lz  = art.z + 0.03f;

        drawQuad(lx1, ly1, lz, lx2, ly1, lz, lx2, ly2, lz, lx1, ly2, lz);

        glColor3f(0.30f, 0.26f, 0.18f);
        float fx1 = art.x - 0.9f;
        float fx2 = art.x + 0.9f;
        float fz1 = -8.3f;
        float fz2 = -7.8f;
        float fy  = 0.01f;

        drawQuad(fx1, fy, fz1, fx2, fy, fz1, fx2, fy, fz2, fx1, fy, fz2);
    }
}

void drawCorridorSigns() {
    glColor3f(0.90f, 0.85f, 0.50f);
    drawText3D(8.8f, 3.4f, 5.0f, "To Architecture Hall ->", 0.0017f);
    drawText3D(15.0f, 3.4f, 5.0f, "<- To Landscape Hall", 0.0017f);

    drawText3D(32.8f, 3.4f, 5.0f, "To Space Hall ->", 0.0017f);
    drawText3D(39.0f, 3.4f, 5.0f, "<- To Architecture Hall", 0.0017f);
}

// -------------------- Interaction --------------------

float distanceToArtwork(const Artwork& art) {
    float dx = camX - art.x;
    float dz = camZ - art.z;
    return std::sqrt(dx * dx + dz * dz);
}

void updateNearestArtwork() {
    nearestArtworkIndex = -1;
    float bestDist = std::numeric_limits<float>::max();

    for (size_t i = 0; i < gallery.size(); i++) {
        float d = distanceToArtwork(gallery[i]);
        if (d < bestDist) {
            bestDist = d;
            nearestArtworkIndex = static_cast<int>(i);
        }
    }

    if (bestDist > 6.0f) {
        nearestArtworkIndex = -1;
    }
}

// -------------------- Art + labels --------------------

void drawFrameBack(float centerX, float centerY, float z, float w, float h, bool highlighted) {
    float x1 = centerX - w / 2.0f;
    float x2 = centerX + w / 2.0f;
    float y1 = centerY - h / 2.0f;
    float y2 = centerY + h / 2.0f;

    if (highlighted) {
        glColor3f(1.0f, 0.85f, 0.20f);
        glLineWidth(6.0f);
    } else {
        glColor3f(0.35f, 0.20f, 0.10f);
        glLineWidth(4.0f);
    }

    glBegin(GL_LINE_LOOP);
        glVertex3f(x1, y1, z + 0.002f);
        glVertex3f(x2, y1, z + 0.002f);
        glVertex3f(x2, y2, z + 0.002f);
        glVertex3f(x1, y2, z + 0.002f);
    glEnd();
}

void drawPaintings() {
    glEnable(GL_TEXTURE_2D);

    for (size_t i = 0; i < gallery.size(); i++) {
        const auto& art = gallery[i];
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
        drawFrameBack(art.x, art.y, art.z, art.drawWidth, art.drawHeight, static_cast<int>(i) == nearestArtworkIndex);
    }

    glDisable(GL_TEXTURE_2D);
}

void drawPaintingLabels() {
    glColor3f(0.15f, 0.10f, 0.08f);

    for (const auto& art : gallery) {
        std::string shortTitle = truncateTitle(art.title, 18);
        drawText3D(art.x - 1.0f, art.y - art.drawHeight / 2.0f - 0.45f, art.z + 0.01f, shortTitle, 0.0015f);
    }
}

void drawRoomTitles() {
    glColor3f(0.20f, 0.16f, 0.12f);

    drawText3D(ROOM1_CENTER_X - 3.0f, 4.9f, 9.7f, "Landscape Hall", 0.0023f);
    drawText3D(ROOM2_CENTER_X - 3.5f, 4.9f, 9.7f, "Architecture Hall", 0.0023f);
    drawText3D(ROOM3_CENTER_X - 2.5f, 4.9f, 9.7f, "Space Hall", 0.0023f);
}

// -------------------- Overlay --------------------

void drawMiniMap() {
    if (!showMiniMap) return;

    const float mapW = 320.0f;
    const float mapH = 140.0f;
    const float mapX = windowWidth - mapW - 20.0f;
    const float mapY = windowHeight - mapH - 20.0f;

    const float worldMinX = -10.0f;
    const float worldMaxX = 58.0f;
    const float worldMinZ = -10.0f;
    const float worldMaxZ = 10.0f;

    glColor3f(0.08f, 0.08f, 0.10f);
    glBegin(GL_QUADS);
        glVertex2f(mapX, mapY);
        glVertex2f(mapX + mapW, mapY);
        glVertex2f(mapX + mapW, mapY + mapH);
        glVertex2f(mapX, mapY + mapH);
    glEnd();

    glColor3f(0.85f, 0.85f, 0.85f);
    glLineWidth(2.0f);

    auto worldToMiniMap = [&](float worldX, float worldZ, float& outX, float& outY) {
        float nx = (worldX - worldMinX) / (worldMaxX - worldMinX);
        float nz = (worldZ - worldMinZ) / (worldMaxZ - worldMinZ);
        outX = mapX + nx * mapW;
        outY = mapY + mapH - (nz * mapH);
    };

    auto drawWorldRect = [&](float x1, float z1, float x2, float z2) {
        float sx1, sy1, sx2, sy2;
        worldToMiniMap(x1, z1, sx1, sy1);
        worldToMiniMap(x2, z2, sx2, sy2);

        float left   = std::min(sx1, sx2);
        float right  = std::max(sx1, sx2);
        float top    = std::min(sy1, sy2);
        float bottom = std::max(sy1, sy2);

        glBegin(GL_LINE_LOOP);
            glVertex2f(left,  top);
            glVertex2f(right, top);
            glVertex2f(right, bottom);
            glVertex2f(left,  bottom);
        glEnd();
    };

    drawWorldRect(-10.0f, -10.0f, 10.0f, 10.0f);    // Room 1
    drawWorldRect(10.0f, DOOR_Z1, 14.0f, DOOR_Z2);  // Corridor 1
    drawWorldRect(14.0f, -10.0f, 34.0f, 10.0f);     // Room 2
    drawWorldRect(34.0f, DOOR_Z1, 38.0f, DOOR_Z2);  // Corridor 2
    drawWorldRect(38.0f, -10.0f, 58.0f, 10.0f);     // Room 3

    glColor3f(0.65f, 0.65f, 0.65f);
    drawWorldRect(-1.0f, -2.5f, 1.0f, -0.5f);
    drawWorldRect(23.0f, -2.5f, 25.0f, -0.5f);
    drawWorldRect(47.0f, -2.5f, 49.0f, -0.5f);

    glColor3f(0.35f, 0.75f, 1.0f);
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < gallery.size(); i++) {
        float ax, ay;
        worldToMiniMap(gallery[i].x, gallery[i].z, ax, ay);
        glVertex2f(ax, ay);
    }
    glEnd();

    if (nearestArtworkIndex >= 0) {
        float ax, ay;
        worldToMiniMap(gallery[nearestArtworkIndex].x, gallery[nearestArtworkIndex].z, ax, ay);
        glColor3f(0.20f, 0.95f, 0.30f);
        glPointSize(8.0f);
        glBegin(GL_POINTS);
            glVertex2f(ax, ay);
        glEnd();
    }

    float px, py;
    worldToMiniMap(camX, camZ, px, py);
    glColor3f(0.95f, 0.30f, 0.30f);
    glPointSize(8.0f);
    glBegin(GL_POINTS);
        glVertex2f(px, py);
    glEnd();

    glColor3f(0.95f, 0.90f, 0.40f);
    glBegin(GL_LINES);
        glVertex2f(px, py);
        glVertex2f(px + std::sin(camYaw) * 12.0f, py - std::cos(camYaw) * 12.0f);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    drawBitmapText2D(mapX + 10.0f, mapY + mapH - 18.0f, "Mini Map");
    drawBitmapText2D(mapX + 10.0f, mapY + 16.0f, "Blue=art  Green=nearest  Red=you");
}

void drawInfoPanel() {
    if (!showInfoPanel || nearestArtworkIndex < 0) return;

    const Artwork& art = gallery[nearestArtworkIndex];

    glColor3f(0.08f, 0.08f, 0.10f);
    glBegin(GL_QUADS);
        glVertex2f(680.0f, 500.0f);
        glVertex2f(1160.0f, 500.0f);
        glVertex2f(1160.0f, 770.0f);
        glVertex2f(680.0f, 770.0f);
    glEnd();

    glColor3f(0.95f, 0.85f, 0.35f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(680.0f, 500.0f);
        glVertex2f(1160.0f, 500.0f);
        glVertex2f(1160.0f, 770.0f);
        glVertex2f(680.0f, 770.0f);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    drawBitmapText2D(700.0f, 740.0f, "Artwork Details");
    drawBitmapText2D(700.0f, 705.0f, "Title: " + art.title);
    drawBitmapText2D(700.0f, 675.0f, "Category: " + art.category);
    drawBitmapText2D(700.0f, 645.0f, "Source: " + art.source);
    drawBitmapText2D(700.0f, 615.0f, "License: " + art.license);
    drawBitmapText2D(700.0f, 585.0f, "Room: " + std::to_string(art.room));
    drawBitmapText2D(700.0f, 555.0f, "Description: " + truncateLine(art.description, 42));
    drawBitmapText2D(700.0f, 525.0f, "Press E to close");
}

void drawOverlay() {
    if (!showHUD && !showMiniMap && !showInfoPanel) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    if (showHUD) {
        glColor3f(1.0f, 1.0f, 1.0f);

        drawBitmapText2D(20.0f, windowHeight - 30.0f, "WASD: move   J/L: turn   E: inspect   M: minimap   H: HUD   ESC: quit");
        drawBitmapText2D(20.0f, windowHeight - 55.0f, "Current Room: " + currentRoomName());

        if (nearestArtworkIndex >= 0) {
            std::string msg = "Nearest: " + truncateTitle(gallery[nearestArtworkIndex].title, 28);
            drawBitmapText2D(20.0f, windowHeight - 80.0f, msg);
            if (!showInfoPanel) {
                drawBitmapText2D(20.0f, windowHeight - 105.0f, "Press E to inspect");
            }
        }

        float cx = windowWidth / 2.0f;
        float cy = windowHeight / 2.0f;
        glBegin(GL_LINES);
            glVertex2f(cx - 8.0f, cy); glVertex2f(cx + 8.0f, cy);
            glVertex2f(cx, cy - 8.0f); glVertex2f(cx, cy + 8.0f);
        glEnd();
    }

    drawMiniMap();
    drawInfoPanel();

    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// -------------------- Main render --------------------

void display() {
    updateNearestArtwork();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float lookX = camX + std::sin(camYaw);
    float lookZ = camZ - std::cos(camYaw);

    gluLookAt(camX, camY, camZ, lookX, camY, lookZ, 0.0f, 1.0f, 0.0f);

    drawRoom();
    drawPaintingSpotlights();
    drawCorridorSigns();
    drawPaintings();
    drawPaintingLabels();
    drawRoomTitles();
    drawOverlay();

    glutSwapBuffers();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    windowWidth = w;
    windowHeight = h;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(65.0f, static_cast<float>(w) / static_cast<float>(h), 0.1f, 220.0f);
    glMatrixMode(GL_MODELVIEW);
}

void clampCamera() {
    if (camX < -8.7f) camX = -8.7f;
    if (camX > 56.7f) camX = 56.7f;
    if (camZ < -8.0f) camZ = -8.0f;
    if (camZ > 8.7f) camZ = 8.7f;

    // Pedestals
    if (camX > -1.5f && camX < 1.5f && camZ > -3.0f && camZ < 0.3f) {
        camX = prevCamX;
        camZ = prevCamZ;
    }
    if (camX > 22.5f && camX < 25.5f && camZ > -3.0f && camZ < 0.3f) {
        camX = prevCamX;
        camZ = prevCamZ;
    }
    if (camX > 46.5f && camX < 49.5f && camZ > -3.0f && camZ < 0.3f) {
        camX = prevCamX;
        camZ = prevCamZ;
    }

    // Room1 -> Corridor1 wall at x ~ 10
    if (camX > 9.6f && camX < 10.4f) {
        if (camZ < DOOR_Z1 || camZ > DOOR_Z2) {
            camX = prevCamX;
            camZ = prevCamZ;
        }
    }

    // Corridor1 -> Room2 wall at x ~ 14
    if (camX > 13.6f && camX < 14.4f) {
        if (camZ < DOOR_Z1 || camZ > DOOR_Z2) {
            camX = prevCamX;
            camZ = prevCamZ;
        }
    }

    // Room2 -> Corridor2 wall at x ~ 34
    if (camX > 33.6f && camX < 34.4f) {
        if (camZ < DOOR_Z1 || camZ > DOOR_Z2) {
            camX = prevCamX;
            camZ = prevCamZ;
        }
    }

    // Corridor2 -> Room3 wall at x ~ 38
    if (camX > 37.6f && camX < 38.4f) {
        if (camZ < DOOR_Z1 || camZ > DOOR_Z2) {
            camX = prevCamX;
            camZ = prevCamZ;
        }
    }

    // Corridor 1 side walls
    if (camX >= 10.0f && camX <= 14.0f) {
        if (camZ < DOOR_Z1 + 0.2f || camZ > DOOR_Z2 - 0.2f) {
            camX = prevCamX;
            camZ = prevCamZ;
        }
    }

    // Corridor 2 side walls
    if (camX >= 34.0f && camX <= 38.0f) {
        if (camZ < DOOR_Z1 + 0.2f || camZ > DOOR_Z2 - 0.2f) {
            camX = prevCamX;
            camZ = prevCamZ;
        }
    }
}

void updateMovement() {
    float move = 0.12f;
    float turn = 0.035f;

    prevCamX = camX;
    prevCamZ = camZ;

    bool moved = false;

    if (keyStates['w'] || keyStates['W']) {
        camX += std::sin(camYaw) * move;
        camZ -= std::cos(camYaw) * move;
        moved = true;
    }
    if (keyStates['s'] || keyStates['S']) {
        camX -= std::sin(camYaw) * move;
        camZ += std::cos(camYaw) * move;
        moved = true;
    }
    if (keyStates['a'] || keyStates['A']) {
        camX -= std::cos(camYaw) * move;
        camZ -= std::sin(camYaw) * move;
        moved = true;
    }
    if (keyStates['d'] || keyStates['D']) {
        camX += std::cos(camYaw) * move;
        camZ += std::sin(camYaw) * move;
        moved = true;
    }
    if (keyStates['j'] || keyStates['J']) {
        camYaw -= turn;
    }
    if (keyStates['l'] || keyStates['L']) {
        camYaw += turn;
    }

    if (moved) {
        clampCamera();
    }

    glutPostRedisplay();
}

void idle() {
    updateMovement();
}

void keyboardDown(unsigned char key, int, int) {
    keyStates[key] = true;

    switch (key) {
        case 'e':
        case 'E':
            if (nearestArtworkIndex >= 0) {
                showInfoPanel = !showInfoPanel;
            }
            break;

        case 'm':
        case 'M':
            showMiniMap = !showMiniMap;
            break;

        case 'h':
        case 'H':
            showHUD = !showHUD;
            break;

        case 27:
            std::exit(0);
    }

    glutPostRedisplay();
}

void keyboardUp(unsigned char key, int, int) {
    keyStates[key] = false;
}

void init() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.15f, 0.15f, 0.18f, 1.0f);

    gallery = loadGallery("data/gallery.json", 15);
    prepareArtworks();

    std::cout << "Prepared " << gallery.size() << " paintings.\n";
    std::cout << "Explore three connected rooms.\n";
    std::cout << "Press E near a painting to inspect it.\n";
}

int main(int argc, char** argv) {
    try {
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
        glutInitWindowSize(windowWidth, windowHeight);
        glutCreateWindow("3D Art Gallery - Three Rooms");

        init();

        glutDisplayFunc(display);
        glutReshapeFunc(reshape);
        glutKeyboardFunc(keyboardDown);
        glutKeyboardUpFunc(keyboardUp);
        glutIdleFunc(idle);

        glutMainLoop();
    } catch (const std::exception& e) {
        std::cerr << "Startup error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
