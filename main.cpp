//
//
//
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/glut.h>
#include <iostream>

using namespace std;

#define MAX_PARTICLES 1000
#define WCX        640
#define WCY        480
#define RAIN    0
#define SNOW    1
#define    HAIL    2


float slowdown = 0.025;
float velocity = 0.0;
float zoom = -40.0;
float pan = 0.0;
float tilt = 0.0;
float hailsize = 0.1;

int loop;
int fall;

//floor colors
float r = 0.0;
float g = 1.0;
float b = 0.0;

float ground_points[21][21][3];
float ground_colors[21][21][4];
float accum = -10.0;

float faceparts[6][3];

typedef struct {
    // Life
    bool alive;    // is the particle alive?
    float life;    // particle lifespan
    float fade; // decay
    // color
    float red;
    float green;
    float blue;
    // Position/direction
    float xpos;
    float ypos;
    float zpos;
    // Velocity/Direction, only goes down in y dir
    float vel;
    float vec[4];
    // Gravity
    float gravity;
}particles;

typedef struct {
    // Life
    float a[3];
    float b[3];
    float c[3];
    float s1[3];
    float s2[3];
    float fnormal[3];
}face;


// Paticle System
particles par_sys[MAX_PARTICLES];


float thefloor[] = {
    0.5f,  -10.0f,  0.5f,
    -0.5f,  -10.0f,  0.5f,
    -0.5f,  -10.0f,  -0.5f,
    0.5f,  -10.0f,  0.5f
};
float d[3] = {0.5f, -15.0f,  0.5f};
float e[3] = {-0.5f, -15.0f,  0.5f};
float f[3] = {-0.5f, -15.0f,  -0.5f};




void init_face( float a[3], float b[3], float c[3]){
    faceparts[0][0] = a[0];
    faceparts[0][1] = a[1];
    faceparts[0][2] = a[2];
    faceparts[1][0] = b[0];
    faceparts[1][1] = b[1];
    faceparts[1][2] = b[2];
    faceparts[2][0] = c[0];
    faceparts[2][1] = c[1];
    faceparts[2][2] = c[2];
    float s1[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    
    faceparts[3][0] = s1[0];
    faceparts[3][1] = s1[1];
    faceparts[3][2] = s1[2];
    float s2[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
    
    faceparts[4][0] = s2[0];
    faceparts[4][1] = s2[1];
    faceparts[4][2] = s2[2];
    float fnormal[3] = {s1[1] * s2[2] - s1[2] * s2[1], s1[2] * s2[0] - s1[0] * s2[2],
        s1[0] * s2[1] - s1[1] * s2[0]};
    
    faceparts[5][0] = fnormal[0];
    faceparts[5][1] = fnormal[1];
    faceparts[5][2] = fnormal[2];
};




bool intersect(float ypos, float face[6][3], float variant) {
    if (ypos*face[5][1] + variant/3 -4.5 > 0) {
        return true;
    }
    return false;
}

void normal_keys(unsigned char key, int x, int y) {
    if (key == 'r') { // Rain
        fall = RAIN;
        glutPostRedisplay();
    }
    if (key == '=') { //really '+' - make hail bigger
        hailsize += 0.01;
    }
    if (key == '-') { // make hail smaller
        if (hailsize > 0.1) hailsize -= 0.01;
    }
    if (key == ',') { // really '<' slow down
        if (slowdown > 4.0) slowdown += 0.01;
    }
    if (key == '.') { // really '>' speed up
        if (slowdown > 1.0) slowdown -= 0.01;
    }
    if (key == 'q') { // QUIT
        exit(0);
    }
}

void special_keys(int key, int x, int y) {
    if (key == GLUT_KEY_UP) {
        zoom += 1.0;
    }
    if (key == GLUT_KEY_DOWN) {
        zoom -= 1.0;
    }
    if (key == GLUT_KEY_RIGHT) {
        pan += 1.0;
    }
    if (key == GLUT_KEY_LEFT) {
        pan -= 1.0;
    }
    if (key == 't') {
        tilt -= 1.0;
    }
    if (key == 'y') {
        tilt += 1.0;
    }
}


// Initialize/Reset Particles - give them their attributes
void initParticles(int i) {
    par_sys[i].alive = true;
    par_sys[i].life = 1.0;
    par_sys[i].fade = float(rand()%100)/1000.0f+0.003f;
    
    par_sys[i].xpos = (float) (rand() % 21) - 10;
    par_sys[i].ypos = 25.0;
    par_sys[i].zpos = (float) (rand() % 21) - 10;
    
    par_sys[i].red = 0.5;
    par_sys[i].green = 0.5;
    par_sys[i].blue = 1.0;
    
    par_sys[i].vel = velocity;
    
    par_sys[i].vec[1] = velocity;
    par_sys[i].gravity = -0.8;//-0.8;
    
}

void init( ) {
    int x, z;
    
    glShadeModel(GL_SMOOTH);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearDepth(1.0);
    glEnable(GL_DEPTH_TEST);
    
    // Ground Verticies
    // Ground Colors
    for (z = 0; z < 21; z++) {
        for (x = 0; x < 21; x++) {
            ground_points[x][z][0] = x - 10.0;
            ground_points[x][z][1] = x/3-7;
            ground_points[x][z][2] = z - 10;
            
            ground_colors[z][x][0] = r; // red value
            ground_colors[z][x][1] = g; // green value
            ground_colors[z][x][2] = b; // blue value
            ground_colors[z][x][3] = 0.0; // acummulation factor
        }
    }
    
    // Initialize particles
    for (loop = 0; loop < MAX_PARTICLES; loop++) {
        initParticles(loop);
    }
}





// For Rain
void drawRain() {
    float x, y, z;
    for (loop = 0; loop < MAX_PARTICLES; loop=loop+2) {
        if (par_sys[loop].alive == true) {
            x = par_sys[loop].xpos;
            y = par_sys[loop].ypos;
            z = par_sys[loop].zpos + zoom;
            
            // Draw particles
            glColor3f(0.5, 0.5, 1.0);
            glBegin(GL_LINES);
            glVertex3f(x, y, z);
            glVertex3f(x, y+0.5, z);
            glEnd();
            
            // Update values
            //Move
            // Adjust slowdown for speed!
            par_sys[loop].ypos += par_sys[loop].vel / (slowdown*1000);
            par_sys[loop].vel += par_sys[loop].gravity;
            // Decay
            par_sys[loop].life -= par_sys[loop].fade;
            
            if (par_sys[loop].ypos < -15 || intersect(par_sys[loop].ypos, faceparts, par_sys[loop].xpos)) {
                par_sys[loop].life = -1.0;
            }
        }
        //Revive
        if (par_sys[loop].life < 0.0) {
            initParticles(loop);
        }
    }
}





// Draw Particles
void drawScene( ) {
    int i, j;
    float x, y, z;
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    
    glLoadIdentity();
    
    
    glRotatef(pan, 0.0, 1.0, 0.0);
    glRotatef(tilt, 1.0, 0.0, 0.0);
    
    // GROUND?!
    glColor3f(r, g, b);
    if (fall == RAIN) {
        drawRain();
    }
    glBegin(GL_QUADS);
    // along z - y const
    for (i = -10; i+1 < 11; i++) {
        // along x - y const
        for (j = -10; j+1 < 11; j++) {
            glColor3fv(ground_colors[i+10][j+10]);
            glVertex3f(ground_points[j+10][i+10][0],
                       ground_points[j+10][i+10][1],
                       ground_points[j+10][i+10][2] + zoom);
            glColor3fv(ground_colors[i+10][j+1+10]);
            glVertex3f(ground_points[j+1+10][i+10][0],
                       ground_points[j+1+10][i+10][1],
                       ground_points[j+1+10][i+10][2] + zoom);
            glColor3fv(ground_colors[i+1+10][j+1+10]);
            glVertex3f(ground_points[j+1+10][i+1+10][0],
                       ground_points[j+1+10][i+1+10][1],
                       ground_points[j+1+10][i+1+10][2] + zoom);
            glColor3fv(ground_colors[i+1+10][j+10]);
            glVertex3f(ground_points[j+10][i+1+10][0],
                       ground_points[j+10][i+1+10][1],
                       ground_points[j+10][i+1+10][2] + zoom);
        }
    }
    
    
    glEnd();
    
    glutSwapBuffers();
}



void reshape(int w, int h) {
    if (h == 0) h = 1;
    
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    gluPerspective(45, (float) w / (float) h, .1, 200);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void idle ( ) {
    glutPostRedisplay();
}

int main (int argc, char** argv) {
    
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(WCX, WCY);
    glutCreateWindow("CMPS 161 - Final Project");
    init();
    
    init_face(d, e, f);
    glutDisplayFunc(drawScene);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(normal_keys);
    glutSpecialFunc(special_keys);
    glutIdleFunc(idle);
    glutMainLoop();
    return 0;
}
