#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

// Camera
float camX = 0.0f;
float camY = 2.0f;
float camZ = 8.0f;
float camYaw = 0.0f;

GLuint paintingTexture = 0;

GLuint loadTexture(const char *path) {
  int width, height, channels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char *data = stbi_load(path, &width, &height, &channels, 0);

  if (!data) {
    std::cerr << "Failed to load image: " << path << "\n";
    return 0;
  }

  GLenum format = GL_RGB;
  if (channels == 1)
    format = GL_RED;
  else if (channels == 3)
    format = GL_RGB;
  else if (channels == 4)
    format = GL_RGBA;

  GLuint textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
               GL_UNSIGNED_BYTE, data);

  glBindTexture(GL_TEXTURE_2D, 0);
  stbi_image_free(data);

  std::cout << "Loaded texture: " << path << " (" << width << "x" << height
            << ")\n";
  return textureID;
}

void drawRoom() {
  // Floor
  glColor3f(0.7f, 0.7f, 0.7f);
  glBegin(GL_QUADS);
  glVertex3f(-10.0f, 0.0f, -10.0f);
  glVertex3f(10.0f, 0.0f, -10.0f);
  glVertex3f(10.0f, 0.0f, 10.0f);
  glVertex3f(-10.0f, 0.0f, 10.0f);
  glEnd();

  // Back wall
  glColor3f(0.9f, 0.9f, 0.85f);
  glBegin(GL_QUADS);
  glVertex3f(-10.0f, 0.0f, -10.0f);
  glVertex3f(10.0f, 0.0f, -10.0f);
  glVertex3f(10.0f, 6.0f, -10.0f);
  glVertex3f(-10.0f, 6.0f, -10.0f);
  glEnd();

  // Left wall
  glBegin(GL_QUADS);
  glVertex3f(-10.0f, 0.0f, -10.0f);
  glVertex3f(-10.0f, 0.0f, 10.0f);
  glVertex3f(-10.0f, 6.0f, 10.0f);
  glVertex3f(-10.0f, 6.0f, -10.0f);
  glEnd();

  // Right wall
  glBegin(GL_QUADS);
  glVertex3f(10.0f, 0.0f, -10.0f);
  glVertex3f(10.0f, 0.0f, 10.0f);
  glVertex3f(10.0f, 6.0f, 10.0f);
  glVertex3f(10.0f, 6.0f, -10.0f);
  glEnd();
}

void drawPaintingFrame(float x1, float y1, float x2, float y2, float z) {
  glColor3f(0.35f, 0.2f, 0.1f); // brown frame
  glLineWidth(6.0f);
  glBegin(GL_LINE_LOOP);
  glVertex3f(x1, y1, z);
  glVertex3f(x2, y1, z);
  glVertex3f(x2, y2, z);
  glVertex3f(x1, y2, z);
  glEnd();
}

void drawPainting() {
  if (paintingTexture == 0)
    return;

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, paintingTexture);

  glColor3f(1.0f, 1.0f, 1.0f);

  // Painting position on back wall
  float x1 = -2.0f;
  float x2 = 2.0f;
  float y1 = 1.5f;
  float y2 = 4.2f;
  float z = -9.99f; // slightly in front of back wall

  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f);
  glVertex3f(x1, y1, z);
  glTexCoord2f(1.0f, 0.0f);
  glVertex3f(x2, y1, z);
  glTexCoord2f(1.0f, 1.0f);
  glVertex3f(x2, y2, z);
  glTexCoord2f(0.0f, 1.0f);
  glVertex3f(x1, y2, z);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);

  drawPaintingFrame(x1, y1, x2, y2, z + 0.001f);
}

void display() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  float lookX = camX + std::sin(camYaw);
  float lookZ = camZ - std::cos(camYaw);

  gluLookAt(camX, camY, camZ, lookX, camY, lookZ, 0.0f, 1.0f, 0.0f);

  drawRoom();
  drawPainting();

  glutSwapBuffers();
}

void reshape(int w, int h) {
  if (h == 0)
    h = 1;
  float aspect = static_cast<float>(w) / static_cast<float>(h);

  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0, aspect, 0.1, 100.0);
  glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int, int) {
  float moveSpeed = 0.4f;
  float turnSpeed = 0.08f;

  switch (key) {
  case 'w':
    camX += std::sin(camYaw) * moveSpeed;
    camZ -= std::cos(camYaw) * moveSpeed;
    break;
  case 's':
    camX -= std::sin(camYaw) * moveSpeed;
    camZ += std::cos(camYaw) * moveSpeed;
    break;
  case 'a':
    camX -= std::cos(camYaw) * moveSpeed;
    camZ -= std::sin(camYaw) * moveSpeed;
    break;
  case 'd':
    camX += std::cos(camYaw) * moveSpeed;
    camZ += std::sin(camYaw) * moveSpeed;
    break;
  case 'j':
    camYaw -= turnSpeed;
    break;
  case 'l':
    camYaw += turnSpeed;
    break;
  case 27:
    std::exit(0);
    break;
  }

  glutPostRedisplay();
}

void init() {
  glEnable(GL_DEPTH_TEST);
  glClearColor(0.15f, 0.15f, 0.18f, 1.0f);

  // Change this to one image that really exists in your assets folder
  paintingTexture = loadTexture("assets/art_35.jpg");
}

int main(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(1000, 700);
  glutCreateWindow("3D Art Gallery");

  init();

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);

  glutMainLoop();
  return 0;
}
