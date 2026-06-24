#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/*
  Rubric / متطلبات OpenGL (تغطية واضحة في الكود):
  Transformations: glTranslatef, glRotatef, glScalef
  Animation: glutTimerFunc + تغيير زوايا المدار والدوران
  Colors: glColor / glMaterial — أجسام متعددة بألوان مختلفة
  Textures: Real image textures loaded using stb_image + glTexImage2D + gluQuadricTexture
  Lighting & shading: GL_LIGHT0; مفتاح F يبدّل glShadeModel(SMOOTH/FLAT)
  Interaction: keyboard + mouse (سحب، عجلة، حركة سلبية/معلومات)
  Extras: كاميرات، مدارات، حلقات، قمر، إيقاف، تلميح عند المؤشر، إلخ.
*/
const float PI = 3.14159265f;

struct Planet {
    std::string name;
    float radius;
    float distance;
    float orbitSpeed;
    float rotationSpeed;
    float orbitAngle;
    float selfAngle;
    float tilt;
    float color[3];
    bool hasRing;
    bool hasMoon;
    GLuint texture;
    std::string fact;
    float yearEarthDays;
};

std::vector<Planet> planets;

GLuint sunTexture, spaceTexture, moonTexture;
int selectedPlanet = -1;
int cameraMode = 0;

bool paused = false;
bool showOrbits = true;
bool smoothShading = true;
bool showInfo = true;
bool showTextures = true;

float animationSpeed = 1.0f;
float cameraDistance = 30.0f;
float cameraAngleX = 25.0f;
float cameraAngleY = -35.0f;
float cameraPanX = 0.0f;
float cameraPanZ = 0.0f;
int spaceTheme = 0;

int lastMouseX, lastMouseY;
int cursorX = 0;
int cursorY = 0;
bool mouseLeftDown = false;

// -1: none, 0..7: planet index, 8: sun
int hoveredId = -1;
int g_winW = 1000;
int g_winH = 700;

const char* sunFact =
"The Sun: G-type main-sequence star; 99.86% of the Solar System's mass. "
"Nuclear fusion of hydrogen; surface ~5,500 C (simplified in this view).";

float degToRad(float d) {
    return d * PI / 180.0f;
}

GLuint loadTextureFromFile(const char* filename) {
    int width, height, channels;

    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);

    if (!data) {
        std::cout << "Failed to load texture: " << filename << std::endl;
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 4)
        format = GL_RGBA;

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        format,
        width,
        height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        data
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(data);
    return textureID;
}

void initTexturesAndPlanets() {
    sunTexture = loadTextureFromFile("textures/2k_sun.jpg");
    spaceTexture = loadTextureFromFile("textures/2k_stars.jpg");
    moonTexture = loadTextureFromFile("textures/2k_moon.jpg");

    GLuint mercury = loadTextureFromFile("textures/2k_mercury.jpg");
    GLuint venus = loadTextureFromFile("textures/2k_venus_surface.jpg");
    GLuint earth = loadTextureFromFile("textures/2k_earth_daymap.jpg");
    GLuint mars = loadTextureFromFile("textures/2k_mars.jpg");
    GLuint jupiter = loadTextureFromFile("textures/2k_jupiter.jpg");
    GLuint saturn = loadTextureFromFile("textures/2k_saturn.jpg");
    GLuint uranus = loadTextureFromFile("textures/2k_uranus.jpg");
    GLuint neptune = loadTextureFromFile("textures/2k_neptune.jpg");

    planets.push_back({ "Mercury", 0.30f, 3.0f, 0, 2.0f, 0, 0, 2.0f,   {0.6f, 0.6f, 0.6f},
        false, false, mercury,
        "Smallest major planet. Cratered surface, almost no air; Sun-facing side bakes, dark side cools. Day ~59 Earth days.",
        88.0f });
    planets.push_back({ "Venus", 0.48f, 4.0f, 0, 0.7f, 0, 0, 177.3f, {0.9f, 0.6f, 0.2f},
        false, false, venus,
        "Thick CO2 clouds; runaway greenhouse ~470 C. Retrograde, very slow day (longer than its year in Earth days in real data).",
        225.0f });
    planets.push_back({ "Earth", 0.50f, 5.0f, 0, 5.0f, 0, 0, 23.4f, {0.2f, 0.5f, 1.0f},
        false, true, earth,
        "Only known life. Nitrogen-oxygen air; water oceans. One large Moon; axial tilt ~23.4 gives seasons. Year ~365.3 days.",
        365.3f });
    planets.push_back({ "Mars", 0.40f, 6.0f, 0, 4.0f, 0, 0, 25.0f, {0.9f, 0.2f, 0.1f},
        false, false, mars,
        "Rusty dust, thin CO2. Ancient river beds suggest past water. Two small moons (Phobos, Deimos) not shown here.",
        687.0f });
    planets.push_back({ "Jupiter", 0.90f, 8.5f, 0, 6.0f, 0, 0, 3.1f, {0.8f, 0.5f, 0.25f},
        false, true, jupiter,
        "Gas giant; Great Red Spot storm. Strong magnetic field, many moons. Galilean moons not modeled here.",
        4300.0f });
    planets.push_back({ "Saturn", 0.80f, 10.0f, 0, 4.0f, 0, 0, 26.7f, {0.9f, 0.75f, 0.35f},
        true, false, saturn,
        "Famous ice rings (wireframe hint). Helium-hydrogen atmosphere; very low mean density. Year ~10,759 Earth days.",
        10759.0f });
    planets.push_back({ "Uranus", 0.60f, 12.0f, 0, 2.2f, 0, 0, 97.8f, {0.4f, 0.9f, 0.9f},
        false, false, uranus,
        "Ice giant; axis nearly on its ecliptic (rolls on its side). Bluish methane. Year ~30,700 Earth days.",
        30687.0f });
    planets.push_back({ "Neptune", 0.58f, 14.0f, 0, 2.2f, 0, 0, 28.3f, {0.1f, 0.2f, 0.9f},
        false, false, neptune,
        "Farthest gas/ice giant; supersonic winds. Deep blue. Year ~60,200 Earth days (Voyager 2 flyby, 1989).",
        60190.0f });

    for (auto& p : planets) {
        // Kepler-like: angular rate ~ 1 / T, with T ~ a^1.5  =>  rate ~ a^-1.5
        const float a = p.distance;
        p.orbitSpeed = 0.9f * std::pow(10.0f / a, 1.5f) * 1.8f;
        if (p.orbitSpeed < 0.12f) p.orbitSpeed = 0.12f;
        if (p.orbitSpeed > 5.0f) p.orbitSpeed = 5.0f;
    }
}

void drawText(float x, float y, const std::string& text) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1000, 0, 700);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glColor3f(1, 1, 1);
    glRasterPos2f(x, y);

    for (char c : text)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);

    glEnable(GL_LIGHTING);

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
}

static int bitmapTextWidth12(const std::string& s) {
    int w = 0;
    for (char ch : s)
        w += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, (int)(unsigned char)ch);
    return w;
}

static int bitmapTextWidth10(const std::string& s) {
    int w = 0;
    for (char ch : s)
        w += glutBitmapWidth(GLUT_BITMAP_HELVETICA_10, (int)(unsigned char)ch);
    return w;
}

static void collectWrappedBlockLines(const std::string& text, int maxC, std::vector<std::string>& out) {
    out.clear();
    size_t i = 0;
    while (i < text.size()) {
        size_t end = i + (size_t)maxC;
        if (end < text.size()) {
            size_t sp = text.rfind(' ', end);
            if (sp != std::string::npos && sp > i + 5) end = sp;
        }
        else end = text.size();
        while (i < end && (text[i] == ' ' || text[i] == '\n')) i++;
        if (i >= end) break;
        out.push_back(text.substr(i, end - i));
        i = end;
    }
}

static void drawTooltipFrameRect(float xL, float yB, float xR, float yT) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1000, 0, 700);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.05f, 0.08f, 0.12f, 0.9f);
    glBegin(GL_QUADS);
    glVertex2f(xL, yB);
    glVertex2f(xR, yB);
    glVertex2f(xR, yT);
    glVertex2f(xL, yT);
    glEnd();

    glColor4f(0.4f, 0.7f, 0.86f, 0.9f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(xL, yB);
    glVertex2f(xR, yB);
    glVertex2f(xR, yT);
    glVertex2f(xL, yT);
    glEnd();

    glDisable(GL_BLEND);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawText12(float x, float y, const std::string& text) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1000, 0, 700);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.92f, 0.92f, 0.92f);
    glRasterPos2f(x, y);

    for (char c : text)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);

    glEnable(GL_LIGHTING);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void drawText10(float x, float y, const std::string& text) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1000, 0, 700);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.88f, 0.9f, 0.92f);
    glRasterPos2f(x, y);

    for (char c : text)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);

    glEnable(GL_LIGHTING);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void drawTextBlock10(float x, float& y, const std::string& text, int maxC, float lineStep) {
    size_t i = 0;
    while (i < text.size()) {
        size_t end = i + (size_t)maxC;
        if (end < text.size()) {
            size_t sp = text.rfind(' ', end);
            if (sp != std::string::npos && sp > i + 5) end = sp;
        }
        else end = text.size();
        while (i < end && (text[i] == ' ' || text[i] == '\n')) i++;
        if (i >= end) break;
        std::string line = text.substr(i, end - i);
        drawText10(x, y, line);
        y -= lineStep;
        i = end;
    }
}

void drawTextBlock12(float x, float& y, const std::string& text) {
    const int maxC = 78;
    size_t i = 0;
    while (i < text.size()) {
        size_t end = i + (size_t)maxC;
        if (end < text.size()) {
            size_t sp = text.rfind(' ', end);
            if (sp != std::string::npos && sp > i + 5) end = sp;
        }
        else end = text.size();
        while (i < end && (text[i] == ' ' || text[i] == '\n')) i++;
        if (i >= end) break;
        std::string line = text.substr(i, end - i);
        drawText12(x, y, line);
        y -= 15.0f;
        i = end;
    }
}

void drawOrbit(float radius) {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glColor3f(0.35f, 0.35f, 0.35f);
    glBegin(GL_LINE_LOOP);

    for (int i = 0; i < 220; i++) {
        float a = 2 * PI * i / 220;
        glVertex3f(cos(a) * radius, 0, sin(a) * radius);
    }

    glEnd();
    glEnable(GL_LIGHTING);
}

void drawSpaceBackground() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    if (showTextures && spaceTexture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, spaceTexture);
    }
    else {
        glDisable(GL_TEXTURE_2D);
    }

    float s = 60.0f;

    glColor3f(1, 1, 1);

    glBegin(GL_QUADS);

    // خلف
    glTexCoord2f(0, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(s, s, -s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, -s);

    // أمام
    glTexCoord2f(0, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(s, s, s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, s);

    // يمين
    glTexCoord2f(0, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(s, s, s);
    glTexCoord2f(0, 1); glVertex3f(s, s, -s);

    // يسار
    glTexCoord2f(0, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(-s, s, s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, -s);

    // فوق
    glTexCoord2f(0, 0); glVertex3f(-s, s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, s, -s);
    glTexCoord2f(1, 1); glVertex3f(s, s, s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, s);

    // تحت
    glTexCoord2f(0, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(s, -s, s);
    glTexCoord2f(0, 1); glVertex3f(-s, -s, s);

    glEnd();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
}

void drawTexturedSphere(float radius, GLuint textureID) {
    GLUquadric* quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);
    gluQuadricNormals(quad, GLU_SMOOTH);

    if (showTextures && textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }
    else {
        glDisable(GL_TEXTURE_2D);
    }

    gluSphere(quad, radius, 48, 48);

    glDisable(GL_TEXTURE_2D);
    gluDeleteQuadric(quad);
}

void drawSunGlow() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glColor4f(1.0f, 0.45f, 0.05f, 0.35f);

    glutWireSphere(1.9f, 32, 32);
    glutWireSphere(2.15f, 32, 32);
    glutWireSphere(2.4f, 32, 32);

    glEnable(GL_LIGHTING);
}

void drawSun() {
    GLfloat emission[] = { 1.0f, 0.45f, 0.05f, 1.0f };
    GLfloat noEmission[] = { 0, 0, 0, 1 };

    glMaterialfv(GL_FRONT, GL_EMISSION, emission);
    glPushMatrix();
    glScalef(1.0f, 0.99f, 1.0f);
    drawTexturedSphere(1.6f, sunTexture);
    glPopMatrix();
    glMaterialfv(GL_FRONT, GL_EMISSION, noEmission);

    if (hoveredId == 8) {
        glDisable(GL_LIGHTING);
        glColor3f(0.25f, 0.9f, 0.95f);
        glutWireSphere(1.72f, 32, 32);
        glEnable(GL_LIGHTING);
    }

    drawSunGlow();
}

void drawPlanetLabel(Planet& p) {
    glDisable(GL_LIGHTING);
    glColor3f(1, 1, 1);
    glRasterPos3f(0, p.radius + 0.4f, 0);

    for (char c : p.name)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);

    glEnable(GL_LIGHTING);
}

void drawPlanet(Planet& p, int index) {
    if (showOrbits)
        drawOrbit(p.distance);

    glPushMatrix();

    glRotatef(p.orbitAngle, 0, 1, 0);
    glTranslatef(p.distance, 0, 0);

    if (index == selectedPlanet && index == hoveredId) {
        glDisable(GL_LIGHTING);
        glColor3f(1, 1, 0);
        glutWireSphere(p.radius + 0.12f, 24, 24);
        glEnable(GL_LIGHTING);
    }
    else if (index == hoveredId) {
        glDisable(GL_LIGHTING);
        glColor3f(0.25f, 0.9f, 0.95f);
        glutWireSphere(p.radius + 0.12f, 24, 24);
        glEnable(GL_LIGHTING);
    }

    glPushMatrix();

    glRotatef(p.tilt, 0, 0, 1);
    glRotatef(p.selfAngle, 0, 1, 0);

    if (index >= 4)
        glScalef(1.08f, 0.97f, 1.0f);

    GLfloat matColor[] = { p.color[0], p.color[1], p.color[2], 1.0f };
    GLfloat specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat shininess[] = { 35.0f };

    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, matColor);
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

    drawTexturedSphere(p.radius, p.texture);

    glPopMatrix();

    if (p.hasRing) {
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);

        glPushMatrix();
        glRotatef(65, 1, 0, 0);
        glScalef(1.15f, 0.45f, 1.15f);
        glColor3f(0.85f, 0.75f, 0.45f);
        glutWireTorus(0.08, p.radius + 0.45f, 20, 100);
        glutWireTorus(0.05, p.radius + 0.70f, 20, 100);
        glPopMatrix();

        glEnable(GL_LIGHTING);
    }

    if (p.hasMoon) {
        glPushMatrix();

        float moonOrbitAngle = p.selfAngle * 2.5f;
        glRotatef(moonOrbitAngle, 0, 1, 0);
        glTranslatef(p.radius + 1.0f, 0, 0);
        glScalef(0.4f, 0.3f, 0.4f);

        GLfloat moonColor[] = { 0.75f, 0.75f, 0.75f, 1.0f };
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, moonColor);

        drawTexturedSphere(p.radius * 0.6f, moonTexture);

        glPopMatrix();
    }

    drawPlanetLabel(p);

    glPopMatrix();
}

void setupLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    GLfloat lightPos[] = { 0, 0, 0, 1 };
    GLfloat ambient[] = { 0.18f, 0.18f, 0.18f, 1 };
    GLfloat diffuse[] = { 1.0f, 0.9f, 0.65f, 1 };
    GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1 };

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
}

void getPlanetWorldPosition(Planet& p, float& x, float& y, float& z) {
    float a = degToRad(p.orbitAngle);
    x = std::cos(a) * p.distance;
    y = 0;
    z = -std::sin(a) * p.distance;
}

void getSelectedPlanetPosition(float& x, float& y, float& z) {
    if (selectedPlanet >= 0 && selectedPlanet < (int)planets.size()) {
        getPlanetWorldPosition(planets[selectedPlanet], x, y, z);
    }
    else {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }
}

void cycleSpaceBackground() {
    // With real image textures, E now toggles textures/background on or off.
    showTextures = !showTextures;
}

void clampCameraPan() {
    const float m = 48.0f;
    if (cameraPanX > m) cameraPanX = m;
    if (cameraPanX < -m) cameraPanX = -m;
    if (cameraPanZ > m) cameraPanZ = m;
    if (cameraPanZ < -m) cameraPanZ = -m;
}

void applyCamera();

static void mapCursorToHud(float& outX, float& outY) {
    int w = (g_winW < 1) ? 1 : g_winW;
    int h = (g_winH < 1) ? 1 : g_winH;
    outX = (float)cursorX * 1000.0f / (float)w;
    outY = (float)(h - 1 - cursorY) * 700.0f / (float)h;
}

void updateHoverPicking() {
    GLdouble model[16], proj[16];
    GLint vp[4];

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    applyCamera();
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetIntegerv(GL_VIEWPORT, vp);

    double bestWz = 2.0;
    int best = -1;

    auto testScreenHit = [&](double wx, double wy, double wz, float radiusWorld, int id) {
        GLdouble sx, sy, sz;
        if (!gluProject(wx, wy, wz, model, proj, vp, &sx, &sy, &sz))
            return;
        if (sz < 0.0 || sz > 1.0)
            return;
        const double yBottom = (double)vp[3] - 1.0 - (double)cursorY;
        const double dx = (double)cursorX - sx;
        const double dy = yBottom - sy;
        const double dist = std::sqrt(dx * dx + dy * dy);
        const double th = 28.0 + 32.0 * (double)radiusWorld;
        if (dist < th) {
            if (best < 0 || (double)sz < bestWz) {
                bestWz = (double)sz;
                best = id;
            }
        }
        };

    const float sunR = 1.68f;
    testScreenHit(0.0, 0.0, 0.0, sunR, 8);

    for (int i = 0; i < (int)planets.size(); i++) {
        float x, y, z;
        getPlanetWorldPosition(planets[i], x, y, z);
        testScreenHit((double)x, (double)y, (double)z, planets[i].radius, i);
    }

    hoveredId = best;
}

void applyCamera() {
    if (cameraMode == 0) {
        glTranslatef(cameraPanX, 0, cameraPanZ);
        glTranslatef(0, 0, -cameraDistance);
        glRotatef(cameraAngleX, 1, 0, 0);
        glRotatef(cameraAngleY, 0, 1, 0);
    }
    else if (cameraMode == 1) {
        gluLookAt(0, 18, 32, 0, 0, 0, 0, 1, 0);
    }
    else if (cameraMode == 2) {
        float x, y, z;
        getSelectedPlanetPosition(x, y, z);
        gluLookAt(x + 3.5f, 2.0f, z + 3.5f, x, y, z, 0, 1, 0);
    }
    else if (cameraMode == 3) {
        gluLookAt(0, 4, 9, 0, 0, 0, 0, 1, 0);
    }
}

void drawHUD() {
    if (hoveredId >= 0) {
        float hx, hy;
        mapCursorToHud(hx, hy);
        const float offX = 14.0f;
        const float offY = 6.0f;
        float x = hx + offX;
        float y = hy - offY;
        if (x > 560.0f)
            x = std::max(8.0f, hx - 300.0f);
        if (x < 5.0f) x = 5.0f;
        if (y < 160.0f) y = 160.0f;
        if (y > 675.0f) y = 675.0f;

        const float padX = 6.0f;
        const float padTop = 8.0f;
        const float padBot = 3.0f;
        const int kTipWrapC = 88;
        const float kTipLine = 12.0f;
        const float kAfterTitle = 14.0f;
        const float kAfterYear = 12.0f;
        int maxW = 0;
        std::vector<std::string> factLines;
        float y0 = y;
        float yLast = y0;

        if (hoveredId == 8) {
            const std::string line0 = "Pointer: Sun (hover)";
            collectWrappedBlockLines(std::string(sunFact), kTipWrapC, factLines);
            maxW = std::max(maxW, bitmapTextWidth10(line0));
            for (const std::string& s : factLines) maxW = std::max(maxW, bitmapTextWidth10(s));
            if (factLines.empty()) yLast = y0;
            else yLast = y0 - kAfterTitle - kTipLine * (float)(factLines.size() - 1);
        }
        else {
            Planet& hp = planets[hoveredId];
            std::stringstream t;
            t << "Pointer: " << hp.name;
            std::string title = t.str();
            t.str("");
            t << "Approx. orbital year: ~" << (int)hp.yearEarthDays << " Earth days (real scale).";
            std::string yl = t.str();
            collectWrappedBlockLines(hp.fact, kTipWrapC, factLines);
            maxW = std::max({ bitmapTextWidth10(title), bitmapTextWidth10(yl), 1 });
            for (const std::string& s : factLines) maxW = std::max(maxW, bitmapTextWidth10(s));
            if (factLines.empty()) yLast = y0 - kAfterTitle;
            else yLast = y0 - kAfterTitle - kAfterYear - kTipLine * (float)(factLines.size() - 1);
        }

        const float yTopF = y0 + padTop;
        const float yBotF = yLast - padBot;
        const float xL = x - padX;
        const float xR = x + (float)maxW + padX;
        const GLboolean wasDepth = glIsEnabled(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        if (wasDepth) glDisable(GL_DEPTH_TEST);
        drawTooltipFrameRect(xL, yBotF, xR, yTopF);

        if (hoveredId == 8) {
            drawText10(x, y, "Pointer: Sun (hover)");
            y -= kAfterTitle;
            drawTextBlock10(x, y, std::string(sunFact), kTipWrapC, kTipLine);
        }
        else {
            Planet& hp = planets[hoveredId];
            std::stringstream t;
            t << "Pointer: " << hp.name;
            drawText10(x, y, t.str());
            y -= kAfterTitle;
            t.str("");
            t << "Approx. orbital year: ~" << (int)hp.yearEarthDays << " Earth days (real scale).";
            drawText10(x, y, t.str());
            y -= kAfterYear;
            drawTextBlock10(x, y, hp.fact, kTipWrapC, kTipLine);
        }

        if (wasDepth) glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
    }

    if (!showInfo) return;

    std::stringstream ss;

    drawText(20, 665, "Interactive 3D Solar System - OpenGL (not to scale, educational)");
    drawText(20, 640, "P: Pause | +/- Speed | O: Orbits | F: Flat/Smooth | T: Textures | I: Info | E: Toggle Textures | 1-8: Planet");
    drawText(20, 615, "Free: WASD move | C: cam | N/B: next/prev | Hover: info near cursor | L-drag: look | wheel: zoom | R: reset");

    if (selectedPlanet >= 0 && selectedPlanet < (int)planets.size()) {
        ss << "Selected Planet: " << planets[selectedPlanet].name;
    }
    else {
        ss << "Selected Planet: None";
    }
    drawText(20, 575, ss.str());

    ss.str("");
    ss << "Camera Mode: ";
    if (cameraMode == 0) ss << "Free Camera";
    if (cameraMode == 1) ss << "Overview Camera";
    if (cameraMode == 2) ss << "Follow Selected Planet";
    if (cameraMode == 3) ss << "Sun Close-up";
    drawText(20, 550, ss.str());

    ss.str("");
    ss << "Speed: " << animationSpeed
        << " | Shading: " << (smoothShading ? "Smooth" : "Flat")
        << " | Textures: " << (showTextures ? "On" : "Off");
    drawText(20, 525, ss.str());

    if (selectedPlanet >= 0 && selectedPlanet < (int)planets.size()) {
        Planet& p = planets[selectedPlanet];

        ss.str("");
        ss.clear();
        ss << "Selected info: r=" << p.radius
            << " | orbit a=" << p.distance
            << " | tilt=" << p.tilt
            << " deg | moon=" << (p.hasMoon ? "Yes" : "No")
            << " | ring=" << (p.hasRing ? "Yes" : "No");
        drawText(20, 500, ss.str());
    }
    else {
        drawText(20, 500, "Selected info: Move mouse over a planet or press 1-8 to select one.");
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    updateHoverPicking();
    glLoadIdentity();

    applyCamera();

    drawSpaceBackground();

    setupLighting();

    if (smoothShading)
        glShadeModel(GL_SMOOTH);
    else
        glShadeModel(GL_FLAT);

    drawSun();

    for (int i = 0; i < (int)planets.size(); i++)
        drawPlanet(planets[i], i);

    drawHUD();

    glutSwapBuffers();
}

void timer(int value) {
    (void)value;
    if (!paused) {
        for (auto& p : planets) {
            p.orbitAngle += p.orbitSpeed * animationSpeed;
            p.selfAngle += p.rotationSpeed * animationSpeed;

            if (p.orbitAngle >= 360) p.orbitAngle -= 360;
            if (p.selfAngle >= 360) p.selfAngle -= 360;
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void keyboard(unsigned char key, int x, int y) {
    (void)x; (void)y;
    switch (key) {
    case 27:
        exit(0);
        break;

    case 'p':
    case 'P':
        paused = !paused;
        break;

    case '+':
        animationSpeed += 0.2f;
        break;

    case '-':
        if (animationSpeed > 0.2f)
            animationSpeed -= 0.2f;
        break;

    case 'o':
    case 'O':
        showOrbits = !showOrbits;
        break;

    case 'f':
    case 'F':
        smoothShading = !smoothShading;
        break;

    case 'e':
    case 'E':
        cycleSpaceBackground();
        break;

    case 'w':
    case 'W':
        if (cameraMode == 0) {
            float r = degToRad(cameraAngleY);
            const float s = 0.38f;
            cameraPanX += s * std::sin(r);
            cameraPanZ += s * std::cos(r);
            clampCameraPan();
        }
        break;
    case 's':
    case 'S':
        if (cameraMode == 0) {
            float r = degToRad(cameraAngleY);
            const float s = 0.38f;
            cameraPanX -= s * std::sin(r);
            cameraPanZ -= s * std::cos(r);
            clampCameraPan();
        }
        break;
    case 'a':
    case 'A':
        if (cameraMode == 0) {
            float r = degToRad(cameraAngleY);
            const float s = 0.38f;
            cameraPanX -= s * std::cos(r);
            cameraPanZ += s * std::sin(r);
            clampCameraPan();
        }
        break;
    case 'd':
    case 'D':
        if (cameraMode == 0) {
            float r = degToRad(cameraAngleY);
            const float s = 0.38f;
            cameraPanX += s * std::cos(r);
            cameraPanZ -= s * std::sin(r);
            clampCameraPan();
        }
        break;

    case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': {
        int k = (int)(key - '1');
        if (k >= 0 && k < (int)planets.size()) {
            selectedPlanet = k;
            cameraMode = 2;
        }
    } break;

    case 'i':
    case 'I':
        showInfo = !showInfo;
        break;

    case 't':
    case 'T':
        showTextures = !showTextures;
        break;

    case 'c':
    case 'C':
        cameraMode = (cameraMode + 1) % 4;
        break;

    case 'n':
    case 'N':
        if (selectedPlanet < 0)
            selectedPlanet = 0;
        else
            selectedPlanet = (selectedPlanet + 1) % planets.size();
        break;

    case 'b':
    case 'B':
        if (selectedPlanet < 0)
            selectedPlanet = (int)planets.size() - 1;
        else {
            selectedPlanet--;
            if (selectedPlanet < 0)
                selectedPlanet = (int)planets.size() - 1;
        }
        break;

    case 'r':
    case 'R':
        cameraMode = 0;
        cameraDistance = 30.0f;
        cameraAngleX = 25.0f;
        cameraAngleY = -35.0f;
        cameraPanX = 0.0f;
        cameraPanZ = 0.0f;
        break;
    }

    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    (void)x; (void)y;
    if (cameraMode == 0) {
        if (key == GLUT_KEY_UP) cameraAngleX -= 3;
        if (key == GLUT_KEY_DOWN) cameraAngleX += 3;
        if (key == GLUT_KEY_LEFT) cameraAngleY -= 3;
        if (key == GLUT_KEY_RIGHT) cameraAngleY += 3;
    }

    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    cursorX = x;
    cursorY = y;
    if (button == GLUT_LEFT_BUTTON) {
        mouseLeftDown = (state == GLUT_DOWN);
        lastMouseX = x;
        lastMouseY = y;
    }

    if (button == 3) {
        cameraDistance -= 1.0f;
        if (cameraDistance < 6.0f) cameraDistance = 6.0f;
    }

    if (button == 4) {
        cameraDistance += 1.0f;
        if (cameraDistance > 70.0f) cameraDistance = 70.0f;
    }

    glutPostRedisplay();
}

void motion(int x, int y) {
    cursorX = x;
    cursorY = y;
    if (mouseLeftDown && cameraMode == 0) {
        cameraAngleY += (x - lastMouseX) * 0.4f;
        cameraAngleX += (y - lastMouseY) * 0.4f;

        lastMouseX = x;
        lastMouseY = y;
    }

    glutPostRedisplay();
}

void passiveMotion(int x, int y) {
    cursorX = x;
    cursorY = y;
    glutPostRedisplay();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;

    g_winW = w;
    g_winH = h;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(45.0, (double)w / h, 1.0, 200.0);

    glMatrixMode(GL_MODELVIEW);
}

void initOpenGL() {
    glClearColor(0.01f, 0.01f, 0.04f, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initTexturesAndPlanets();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1000, 700);
    glutInitWindowPosition(100, 50);

    glutCreateWindow("Interactive 3D Solar System - OpenGL");

    initOpenGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(passiveMotion);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();

    return 0;
}
