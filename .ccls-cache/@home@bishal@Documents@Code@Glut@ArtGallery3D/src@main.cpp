#include <GL/glut.h>
#include <cmath>
#include <iostream>

// Camera position
float camX = 0.0f;
float camY = 2.0f;
float camZ = 8.0f;
float camYaw = 0.0f;

void drawRoom() {
    // Floor
    glColor3f(0.7f, 0.7f, 0.7f);
    glBegin(GL_QUADS);
        glVertex3f(-10.0f, 0.0f, -10.0f);
        glVertex3f( 10.0f, 0.0f, -10.0f);
        glVertex3f( 10.0f, 0.0f,  10.0f);
        glVertex3f(-10.0f, 0.0f,  10.0f);
    glEnd();

    // Back wall
    glColor3f(0.9f, 0.9f, 0.85f);
    glBegin(GL_QUADS);
        glVertex3f(-10.0f, 0.0f, -10.0f);
        glVertex3f( 10.0f, 0.0f, -10.0f);
        glVertex3f( 10.0f, 6.0f, -10.0f);
        glVertex3f(-10.0f, 6.0f, -10.0f);
    glEnd();

    // Left wall
    glBegin(GL_QUADS);
        glVertex3f(-10.0f, 0.0f, -10.0f);
        glVertex3f(-10.0f, 0.0f,  10.0f);
        glVertex3f(-10.0f, 6.0f,  10.0f);
        glVertex3f(-10.0f, 6.0f, -10.0f);
    glEnd();

    // Right wall
    glBegin(GL_QUADS);
        glVertex3f(10.0f, 0.0f, -10.0f);
        glVertex3f(10.0f, 0.0f,  10.0f);
        glVertex3f(10.0f, 6.0f,  10.0f);
        glVertex3f(10.0f, 6.0f, -10.0f);
    glEnd();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float lookX = camX + std::sin(camYaw);
    float lookZ = camZ - std::cos(camYaw);

    gluLookAt(
        camX, camY, camZ,
        lookX, camY, lookZ,
        0.0f, 1.0f, 0.0f
    );

    drawRoom();

    glutSwapBuffers();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
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
        case 27: // ESC
            std::exit(0);
            break;
    }

    glutPostRedisplay();
}

void init() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
}

int main(int argc, char** argv) {
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
