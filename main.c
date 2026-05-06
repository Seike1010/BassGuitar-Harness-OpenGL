#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>

// --- Configuration Constants ---
#define WAIST_RADIUS 0.5f
#define SHOULDER_WIDTH 0.6f
#define SHOULDER_HEIGHT 1.2f
#define STRAP_WIDTH 0.08f
#define STRAP_DEPTH 0.025f
// Centralized floor height
#define FLOOR_LEVEL -0.37f
float camX = 0.0f;
float camY = 0.5f; // Initial Height
float camZ = 0.0f;
float moveSpeed = 0.1f; // Motion sensitivity

int windowWidth = 800;
int windowHeight = 600;

// Global Variables
float angleX = 0.0f;
float angleY = 0.0f;
int lastMouseX, lastMouseY;
int isDragging = 0;

GLuint textureId;
GLfloat light_pos[] = { 4.0f, 6.0f, 4.0f, 1.0f };
float viewDistance = 5.5f;

void drawTitle(const char* title)
{
    //  Backup current 3D Projection Matrix
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    // Switch to 2D Orthographic Projection
    gluOrtho2D(0, windowWidth, 0, windowHeight);

    // Backup ModelView Matrix
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Keep title at front and solid color
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    // Calculate title width for centering
    int textWidth = 0;
    int len = strlen(title);
    void* font = GLUT_BITMAP_TIMES_ROMAN_24;

    for (int i = 0; i < len; i++)
    {
        textWidth += glutBitmapWidth(font, title[i]);
    }

    // Calculate position
    float x = (windowWidth - textWidth) / 2.0f;
    float y = windowHeight - 40.0f;

    // Set color and draw
    glColor3f(0.1f, 0.1f, 0.1f);
    glRasterPos2f(x, y);

    for (int i = 0; i < len; i++)
    {
        glutBitmapCharacter(font, title[i]);
    }

    // Restore 3D Environment
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glPopMatrix(); // Restore Modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix(); // Restore Projection
    glMatrixMode(GL_MODELVIEW);
}

// Math & Helper Functions
// Calculate shadow projection matrix
void shadowMatrix(GLfloat shadowMat[4][4], GLfloat lightpos[4], GLfloat plane[4])
{
    GLfloat dot;
    dot = plane[0] * lightpos[0] + plane[1] * lightpos[1] + plane[2] * lightpos[2] + plane[3] * lightpos[3];

    shadowMat[0][0] = dot - lightpos[0] * plane[0];
    shadowMat[1][0] = 0.f - lightpos[0] * plane[1];
    shadowMat[2][0] = 0.f - lightpos[0] * plane[2];
    shadowMat[3][0] = 0.f - lightpos[0] * plane[3];

    shadowMat[0][1] = 0.f - lightpos[1] * plane[0];
    shadowMat[1][1] = dot - lightpos[1] * plane[1];
    shadowMat[2][1] = 0.f - lightpos[1] * plane[2];
    shadowMat[3][1] = 0.f - lightpos[1] * plane[3];

    shadowMat[0][2] = 0.f - lightpos[2] * plane[0];
    shadowMat[1][2] = 0.f - lightpos[2] * plane[1];
    shadowMat[2][2] = dot - lightpos[2] * plane[2];
    shadowMat[3][2] = 0.f - lightpos[2] * plane[3];

    shadowMat[0][3] = 0.f - lightpos[3] * plane[0];
    shadowMat[1][3] = 0.f - lightpos[3] * plane[1];
    shadowMat[2][3] = 0.f - lightpos[3] * plane[2];
    shadowMat[3][3] = dot - lightpos[3] * plane[3];
}

// Calculate Bezier curve point
void bezierPoint(float t, float p0[3], float p1[3], float p2[3], float out[3])
{
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    for(int i=0; i<3; i++)
    {
        out[i] = uu * p0[i] + 2 * u * t * p1[i] + tt * p2[i];
    }
}

// Generate a procedural leather-like texture
void createTexture()
{
    int w = 64, h = 64;
    unsigned char *data = (unsigned char*)malloc(w * h * 3);
    for (int i = 0; i < w * h; i++)
    {
        int noise = rand() % 40;
        data[i*3+0] = 60 + noise;
        data[i*3+1] = 30 + noise;
        data[i*3+2] = 10 + noise/2;
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    free(data);
}

// Drawing Primitives
// Draw a straight strap segment with thickness
void drawStrapSegment(float p1[3], float p2[3], float width)
{
    float len = sqrt(pow(p2[0]-p1[0],2) + pow(p2[1]-p1[1],2) + pow(p2[2]-p1[2],2));
    float dx = width / 2.0f;
    float dz = STRAP_DEPTH / 2.0f;

    glBegin(GL_QUADS);
        // Front
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f);       glVertex3f(p1[0] - dx, p1[1], p1[2] + dz);
        glTexCoord2f(1.0f, 0.0f);       glVertex3f(p1[0] + dx, p1[1], p1[2] + dz);
        glTexCoord2f(1.0f, len*2.0f);   glVertex3f(p2[0] + dx, p2[1], p2[2] + dz);
        glTexCoord2f(0.0f, len*2.0f);   glVertex3f(p2[0] - dx, p2[1], p2[2] + dz);

        // Back
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);       glVertex3f(p1[0] - dx, p1[1], p1[2] - dz);
        glTexCoord2f(0.0f, 0.0f);       glVertex3f(p1[0] + dx, p1[1], p1[2] - dz);
        glTexCoord2f(0.0f, len*2.0f);   glVertex3f(p2[0] + dx, p2[1], p2[2] - dz);
        glTexCoord2f(1.0f, len*2.0f);   glVertex3f(p2[0] - dx, p2[1], p2[2] - dz);

        // Left Side
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);       glVertex3f(p1[0] - dx, p1[1], p1[2] - dz);
        glTexCoord2f(0.1f, 0.0f);       glVertex3f(p1[0] - dx, p1[1], p1[2] + dz);
        glTexCoord2f(0.1f, len*2.0f);   glVertex3f(p2[0] - dx, p2[1], p2[2] + dz);
        glTexCoord2f(0.0f, len*2.0f);   glVertex3f(p2[0] - dx, p2[1], p2[2] - dz);

        // Right Side
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);       glVertex3f(p1[0] + dx, p1[1], p1[2] + dz);
        glTexCoord2f(0.1f, 0.0f);       glVertex3f(p1[0] + dx, p1[1], p1[2] - dz);
        glTexCoord2f(0.1f, len*2.0f);   glVertex3f(p2[0] + dx, p2[1], p2[2] - dz);
        glTexCoord2f(0.0f, len*2.0f);   glVertex3f(p2[0] + dx, p2[1], p2[2] + dz);
    glEnd();
}

// Draw a curved strap using Bezier interpolation
void drawCurvedStrap(float p0[3], float p1[3], float p2[3], float width)
{
    int segments = 20;
    float t;
    float pt[3];
    float strapLen = 2.5f;
    float dx = width / 2.0f;
    float dz = STRAP_DEPTH / 2.0f;

    // Outer layer
    glBegin(GL_QUAD_STRIP);
    glNormal3f(0.0f, 1.0f, 0.0f);
    for(int i = 0; i <= segments; i++)
    {
        t = (float)i / segments;
        bezierPoint(t, p0, p1, p2, pt);
        glTexCoord2f(0.0f, t * strapLen); glVertex3f(pt[0] - dx, pt[1], pt[2] + dz);
        glTexCoord2f(1.0f, t * strapLen); glVertex3f(pt[0] + dx, pt[1], pt[2] + dz);
    }
    glEnd();

    // Inner layer
    glBegin(GL_QUAD_STRIP);
    glNormal3f(0.0f, -1.0f, 0.0f);
    for(int i = 0; i <= segments; i++)
    {
        t = (float)i / segments;
        bezierPoint(t, p0, p1, p2, pt);
        glTexCoord2f(1.0f, t * strapLen); glVertex3f(pt[0] + dx, pt[1], pt[2] - dz);
        glTexCoord2f(0.0f, t * strapLen); glVertex3f(pt[0] - dx, pt[1], pt[2] - dz);
    }
    glEnd();
}

// Scene Objects
// Draw the Harness system
void drawHarness()
{
    GLfloat mat_white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_white);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Waist Ring
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

    glPushMatrix();
        glScalef(1.4f, 1.3f, 1.0f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        glutSolidTorus(0.03, WAIST_RADIUS * 0.68, 10, 20);
    glPopMatrix();

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);

    // Strap Calculations
    float z_front = WAIST_RADIUS * 0.55f;
    float z_back  = -WAIST_RADIUS * 0.55f;
    float widthFactor = 0.75f;
    float transH = SHOULDER_HEIGHT * 0.60f;
    float archHeight = SHOULDER_HEIGHT * 1.3f;

    float waistLF[3] = {-WAIST_RADIUS * widthFactor, 0.0f, z_front};
    float waistRF[3] = { WAIST_RADIUS * widthFactor, 0.0f, z_front};
    float waistLB[3] = {-WAIST_RADIUS * widthFactor, 0.0f, z_back};
    float waistRB[3] = { WAIST_RADIUS * widthFactor, 0.0f, z_back};

    float transLF[3] = { waistLF[0], transH, waistLF[2] };
    float transLB[3] = { waistLB[0], transH, waistLB[2] };
    float transRF[3] = { waistRF[0], transH, waistRF[2] };
    float transRB[3] = { waistRB[0], transH, waistRB[2] };

    float ctrlL[3] = { -SHOULDER_WIDTH/2.0f, archHeight, 0.0f };
    float ctrlR[3] = {  SHOULDER_WIDTH/2.0f, archHeight, 0.0f };

    // Draw Straps
    drawStrapSegment(transLF, waistRF, STRAP_WIDTH);
    drawStrapSegment(transRF, waistLF, STRAP_WIDTH);
    drawStrapSegment(transLB, waistRB, STRAP_WIDTH);
    drawStrapSegment(transRB, waistLB, STRAP_WIDTH);

    drawCurvedStrap(transLF, ctrlL, transLB, STRAP_WIDTH);
    drawCurvedStrap(transRF, ctrlR, transRB, STRAP_WIDTH);

    // Vertical Supports
    glDisable(GL_TEXTURE_2D);
    GLfloat mat_dark_silver[] = { 0.25f, 0.25f, 0.3f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_dark_silver);

    float supportLength = 0.4f;
    float centerWaistFront[3] = {0.0f, 0.0f, z_front};
    float supportEndFront[3]  = {0.0f, supportLength, z_front};
    drawStrapSegment(centerWaistFront, supportEndFront, STRAP_WIDTH * 1.2f);

    float centerWaistBack[3]  = {0.0f, 0.0f, z_back};
    float supportEndBack[3]   = {0.0f, supportLength, z_back};
    drawStrapSegment(centerWaistBack, supportEndBack, STRAP_WIDTH * 1.2f);

    // Buckles
    GLfloat mat_silver[] = { 0.6f, 0.6f, 0.7f, 1.0f };
    GLfloat mat_specular[]    = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat mat_emission[]    = { 0.1f, 0.1f, 0.2f, 1.0f };
    GLfloat mat_shiny[]       = { 128.0f };

    glDisable(GL_TEXTURE_2D);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_silver);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_EMISSION, mat_emission);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shiny);

    // Calculate buckle positions based on interpolation
    float t1 = 0.15f;
    float buckleTop[3];
    buckleTop[0] = transRF[0] * (1-t1) + waistLF[0] * t1;
    buckleTop[1] = transRF[1] * (1-t1) + waistLF[1] * t1;
    buckleTop[2] = transRF[2] * (1-t1) + waistLF[2] * t1 + 0.04f;

    float t2 = 0.90f;
    float buckleBottom[3];
    buckleBottom[0] = transRF[0] * (1-t2) + waistLF[0] * t2;
    buckleBottom[1] = transRF[1] * (1-t2) + waistLF[1] * t2;
    buckleBottom[2] = transRF[2] * (1-t2) + waistLF[2] * t2 + 0.04f;

    glPushMatrix();
        glTranslatef(buckleTop[0], buckleTop[1], buckleTop[2]);
        glRotatef(30.0f, 0.0f, 0.0f, 1.0f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        glutSolidTorus(0.015, 0.045, 20, 20);
    glPopMatrix();

    glPushMatrix();
        glTranslatef(buckleBottom[0], buckleBottom[1], buckleBottom[2]);
        glRotatef(30.0f, 0.0f, 0.0f, 1.0f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        glutSolidTorus(0.015, 0.045, 20, 20);
    glPopMatrix();

    // Restore standard materials
    GLfloat no_emission[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat no_specular[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_EMISSION, no_emission);
    glMaterialfv(GL_FRONT, GL_SPECULAR, no_specular);
}

// Draw the Dummy
void drawDummy()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glPushMatrix();
        glTranslatef(0.0f, 0.55f, 0.0f);
        glScalef(0.8f, 1.2f, 0.5f);
        glutWireCube(1.0f);
    glPopMatrix();

    glPushMatrix();
        glTranslatef(0.0f, 1.43f, 0.0f);
        glutWireSphere(0.3f, 10, 10);
    glPopMatrix();

    glEnable(GL_LIGHTING);
}

// Draw the Bass Guitar
void drawBass(int shadowMode)
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glPushMatrix();
    glScalef(0.8f, 0.8f, 0.8f);

    // Body
    glPushMatrix();
        glPushMatrix();
            glTranslatef(0.0f, -0.3f, 0.0f);
            glScalef(0.7f, 0.7f, 0.12f);
            glutWireCube(1.0f);
        glPopMatrix();
        glPushMatrix();
            glTranslatef(0.0f, 0.25f, 0.0f);
        glScalef(0.55f, 0.5f, 0.12f);
        glutWireCube(1.0f);
        glPopMatrix();
    glPopMatrix();

    // Neck
    glPushMatrix();
        glTranslatef(0.0f, 1.1f, 0.06f);
        glScalef(0.14f, 1.4f, 0.05f);
        glutWireCube(1.0f);
    glPopMatrix();

    // Headstock and Tuners
    glPushMatrix();
        glTranslatef(0.0f, 1.9f, 0.05f);
        glRotatef(-15.0f, 1.0f, 0.0f, 0.0f);
        glPushMatrix();
            glScalef(0.22f, 0.4f, 0.05f);
            glutWireCube(1.0f);
        glPopMatrix();

    for(int i=0; i<4; i++)
    {
        glPushMatrix();
            glTranslatef(-0.15f, -0.18f + i*0.12f, 0.0f);
            glScalef(0.08f, 0.08f, 0.15f);
            glutWireSphere(0.5f, 6, 6);
        glPopMatrix();
    }
    glPopMatrix();

    // Pickups and Bridge
    for(int i=0; i<2; i++)
    {
        glPushMatrix();
            glTranslatef(0.0f, 0.0f + i*0.3f, 0.065f);
            glScalef(0.35f, 0.08f, 0.02f);
            glutWireCube(1.0f);
        glPopMatrix();
    }

    glPushMatrix();
        glTranslatef(0.0f, -0.45f, 0.065f);
        glScalef(0.3f, 0.12f, 0.03f);
        glutWireCube(1.0f);
    glPopMatrix();

    float knobPos[3][3] =
    {
        { 0.25f, -0.42f, 0.065f },
        { 0.22f, -0.52f, 0.065f },
        { 0.16f, -0.60f, 0.065f }
    };

    for(int i=0; i<3; i++)
    {
        glPushMatrix();
            glTranslatef(knobPos[i][0], knobPos[i][1], knobPos[i][2]);
            glutWireSphere(0.04f, 8, 8);
        glPopMatrix();
    }

    // Strings (White if not in shadow mode)
    if (shadowMode == 0)
    {
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    glBegin(GL_LINES);
    for(int i=0; i<4; i++)
    {
        float stringX = -0.045f + i * 0.03f;
        glVertex3f(stringX, -0.45f, 0.08f);
        glVertex3f(stringX, 1.9f, 0.08f);
    }
    glEnd();

    glPopMatrix();
    glEnable(GL_LIGHTING);
}

// Draw the Floor
void drawFloor()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    glColor3f(0.8f, 0.8f, 0.8f);
    glNormal3f(0.0f, 1.0f, 0.0f);

    // Use global constant for floor height
    glVertex3f(-3.0f, FLOOR_LEVEL, 3.0f);
    glVertex3f( 3.0f, FLOOR_LEVEL, 3.0f);
    glVertex3f( 3.0f, FLOOR_LEVEL, -3.0f);
    glVertex3f(-3.0f, FLOOR_LEVEL, -3.0f);
    glEnd();

    glEnable(GL_LIGHTING);
}

// Input & Display
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(camX, camY, viewDistance + camZ,
              camX, camY - 0.5f, camZ,
              0.0f, 1.0f, 0.0f);

    glRotatef(angleX, 1.0f, 0.0f, 0.0f);
    glRotatef(angleY, 0.0f, 1.0f, 0.0f);
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

    // Draw Scene Entities
    drawFloor();

    glColor3f(0.0f, 0.0f, 1.0f);
    drawDummy();

    drawHarness();

    // Calculate Bass Position (Hanging from buckles)
    float z_front = WAIST_RADIUS * 0.55f;
    float transH = SHOULDER_HEIGHT * 0.85f;
    float waistLF[3] = {-WAIST_RADIUS*0.5f, 0.0f, z_front};
    float transRF[3] = { WAIST_RADIUS*0.5f, transH, z_front };

    float t1 = 0.15f;
    float bTop[3];
    bTop[0] = transRF[0] * (1-t1) + waistLF[0] * t1;
    bTop[1] = transRF[1] * (1-t1) + waistLF[1] * t1;
    bTop[2] = transRF[2] * (1-t1) + waistLF[2] * t1 + 0.15f;

    float t2 = 0.90f;
    float bBot[3];
    bBot[0] = transRF[0] * (1-t2) + waistLF[0] * t2;
    bBot[1] = transRF[1] * (1-t2) + waistLF[1] * t2;
    bBot[2] = transRF[2] * (1-t2) + waistLF[2] * t2 + 0.15f;

    glPushMatrix();

    float midX = (bTop[0] + bBot[0]) / 2.0f;
    float midY = (bTop[1] + bBot[1]) / 2.0f;
    float midZ = (bTop[2] + bBot[2]) / 2.0f;

    midX -= 0.2f;
    midY += 0.3f;

    glTranslatef(midX, midY, midZ);

    float dx = bBot[0] - bTop[0];
    float dy = bBot[1] - bTop[1];
    float angle = atan2(dy, dx) * 180.0f / M_PI;

    // Rotate and Draw the Bass
    glRotatef(angle + 73.0f, 0.0f, 0.0f, 1.0f);
    glTranslatef(0.7f, -0.13f, -0.05f);

    glColor3f(1.0f, 0.2f, 0.2f);
    // 0 = Normal mode
    drawBass(0);

    glPopMatrix();

    // Shadow Rendering
    // Create shadow plane slightly above floor to avoid Z-fighting
    GLfloat floorPlane[] = { 0.0f, 1.0f, 0.0f, -FLOOR_LEVEL - 0.001f };
    GLfloat shadowMat[4][4];
    shadowMatrix(shadowMat, light_pos, floorPlane);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);

    glPushMatrix();
    glMultMatrixf((GLfloat *)shadowMat);

    drawHarness();
    drawDummy();

    // Draw Bass Shadow
    glPushMatrix();
        glTranslatef(midX, midY, midZ);
        // Synced with real bass
        glRotatef(angle + 73.0f, 0.0f, 0.0f, 1.0f);
        glTranslatef(0.7f, -0.13f, -0.05f);
        // 1 = Shadow mode
        drawBass(1);
    glPopMatrix();

    glPopMatrix();

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    drawTitle("Guitar Support Harness");

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    if (h == 0)
        h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)w/h, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void mouse(int button, int state, int x, int y)
{
    // Zoom control
    // Scroll Up
    if (button == 3)
    {
        if (state == GLUT_DOWN)
        {
            viewDistance -= 0.5f;
            if (viewDistance < 2.0f)
                viewDistance = 2.0f;
            glutPostRedisplay();
        }
    }
    // Scroll Down
    else if (button == 4)
    {
        if (state == GLUT_DOWN)
        {
            viewDistance += 0.5f;
            if (viewDistance > 20.0f)
                viewDistance = 20.0f;
            glutPostRedisplay();
        }
    }
    // Rotation drag
    else if (button == GLUT_LEFT_BUTTON)
    {
        if (state == GLUT_DOWN)
        {
            isDragging = 1;
            lastMouseX = x;
            lastMouseY = y;
        }
        else
        {
            isDragging = 0;
        }
    }
}

void motion(int x, int y)
{
    if (isDragging)
    {
        angleY += (x - lastMouseX);
        angleX += (y - lastMouseY);
        lastMouseX = x; lastMouseY = y;
        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
        case 'w': case 'W':
            camZ -= moveSpeed;
            break;
        case 's': case 'S':
            camZ += moveSpeed;
            break;
        case 'a': case 'A':
            camX -= moveSpeed;
            break;
        case 'd': case 'D':
            camX += moveSpeed;
            break;
        case 'q': case 'Q':
            camY += moveSpeed;
            break;
        case 'e': case 'E':
            camY -= moveSpeed;
            break;
        case 27:
            exit(0);
            break;
    }
    glutPostRedisplay(); // Refresh
}

// Main Entry Point
void init()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat light_ambient[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    GLfloat light_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

    glClearColor(0.92f, 0.92f, 0.92f, 1.0f);
    glLineWidth(1.5f);
    createTexture();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("CST2309176");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
