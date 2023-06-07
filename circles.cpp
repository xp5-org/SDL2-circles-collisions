#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <random>
#include <cmath>
#include <chrono>
#include <thread>
#include <vector>
#include <iostream>

const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 600;
const int CIRCLE_MIN_RADIUS = 2;
const int CIRCLE_MAX_RADIUS = 32;
const float CIRCLE_SPEED = 1.2;
int MAX_CIRCLES = 400;  // Maximum number of circles
const int FRAMES_PER_SECOND = 30;
const int SPAWN_RATE = 150; // milliseconds to wait between spawning
const float MAX_ACCELERATION = 2.2f;  // Maximum acceleration allowed during collision response
const float MAX_PUSH_DISTANCE = 0.5f;  // Maximum distance a circle can be pushed during collision response
static int AUTOMODE = 1; //enable or disable render lag detector
static int physicsTimer = 40; // number of milliseconds to wait between processing collision physics
const Uint32 DELAY_THRESHOLD = 1000;  // Wait 2 seconds before reducing MAX_CIRCLES
const Uint32 NO_DELAY_THRESHOLD = 5000;  // Wait 5 seconds before increasing MAX_CIRCLES




int originalMaxCircles = MAX_CIRCLES;  // Store the original value of MAX_CIRCLES
const int FRAME_DELAY = 1000 / FRAMES_PER_SECOND;

struct Circle {
    float x;
    float y;
    float radius;
    SDL_Color color;
    float dx;
    float dy;
};

SDL_Window* window = nullptr;
SDL_GLContext glContext;
std::vector<Circle> circles;
SDL_Rect leftWall;
SDL_Rect rightWall;
GLuint vbo;

void initSDL() {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("QCMs Circle Collision Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    glContext = SDL_GL_CreateContext(window);

    glClearColor(0, 0, 0, 1);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    PFNGLGENBUFFERSPROC glGenBuffers;
    glGenBuffers = (PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers");
    glGenBuffers(1, &vbo);
}

Circle createRandomCircle() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> sizeDistribution(CIRCLE_MIN_RADIUS, CIRCLE_MAX_RADIUS);
    std::uniform_int_distribution<int> colorDistribution(0, 255);

    Circle circle;
    circle.radius = sizeDistribution(gen);
    circle.x = std::uniform_int_distribution<int>(circle.radius, SCREEN_WIDTH - circle.radius)(gen);
    circle.y = 0;
    circle.color.r = colorDistribution(gen);
    circle.color.g = colorDistribution(gen);
    circle.color.b = colorDistribution(gen);
    circle.dx = 0;
    circle.dy = CIRCLE_SPEED;

    return circle;
}

void createCircle() {
    static Uint32 lastCircleCreationTime = 0;
    Uint32 currentTime = SDL_GetTicks();

    if (AUTOMODE == 0) {
        if (circles.size() >= MAX_CIRCLES) {
            return;
        }
    }
    else {
        if (circles.size() >= MAX_CIRCLES) {
            while (circles.size() > MAX_CIRCLES) {
                circles.erase(circles.begin());  // Remove the oldest circle
            }
        }
    }

    if (currentTime - lastCircleCreationTime >= SPAWN_RATE) {
        Circle circle = createRandomCircle();
        circles.push_back(circle);

        lastCircleCreationTime = currentTime;  // Update the last creation time
    }
}


bool circlesCollide(const Circle& circle1, const Circle& circle2) {
    float dx = circle1.x - circle2.x;
    float dy = circle1.y - circle2.y;
    float distance = std::sqrt(dx * dx + dy * dy);
    float minDistance = circle1.radius + circle2.radius;
    return distance <= minDistance;
}


void handleCircleCollision(Circle& circle1, Circle& circle2) {
    float dx = circle2.x - circle1.x;
    float dy = circle2.y - circle1.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    // Calculate the minimum translation distance to separate the circles
    float minTranslationDist = distance - circle1.radius - circle2.radius;

    // Calculate the normal vector between the centers of the circles
    float nx = dx / distance;
    float ny = dy / distance;

    // Calculate the relative velocity of the circles
    float relativeVelocityX = circle2.dx - circle1.dx;
    float relativeVelocityY = circle2.dy - circle1.dy;

    // Calculate the impulse magnitude
    float impulseMag = (2.0f * (relativeVelocityX * nx + relativeVelocityY * ny)) /
                       (circle1.radius + circle2.radius);

    // Apply the impulse to the circles based on their masses (sizes)
    float massRatio = circle2.radius / circle1.radius;
    float pushDistance = std::min(minTranslationDist * massRatio, MAX_PUSH_DISTANCE);
    circle1.dx += std::min(impulseMag * nx * massRatio, MAX_ACCELERATION);
    circle1.dy += std::min(impulseMag * ny * massRatio, MAX_ACCELERATION);
    circle2.dx -= std::min(impulseMag * nx / massRatio, MAX_ACCELERATION);
    circle2.dy -= std::min(impulseMag * ny / massRatio, MAX_ACCELERATION);

    // Separate the circles to prevent overlap
    circle1.x += pushDistance * (dx / distance);
    circle1.y += pushDistance * (dy / distance);
    circle2.x -= pushDistance * (dx / distance);
    circle2.y -= pushDistance * (dy / distance);
}

void updateCircles() {
    static Uint32 lastPhysicsUpdateTime = 0;
    Uint32 currentTime = SDL_GetTicks();

    if (currentTime - lastPhysicsUpdateTime >= physicsTimer) {
        for (size_t i = 0; i < circles.size(); ++i) {
            Circle& circle = circles[i];
            circle.x += circle.dx;
            circle.y += circle.dy;

            if (circle.y + circle.radius >= SCREEN_HEIGHT) {
                if (circle.x >= leftWall.x + leftWall.w || circle.x + circle.radius <= rightWall.x) {
                    circle.y = SCREEN_HEIGHT - circle.radius;
                    circle.dx = 0;
                    circle.dy = 0;
                }
            }

            for (size_t j = i + 1; j < circles.size(); ++j) {
                Circle& otherCircle = circles[j];
                if (circlesCollide(circle, otherCircle)) {
                    handleCircleCollision(circle, otherCircle);
                }
            }
        }

        lastPhysicsUpdateTime = currentTime;
    }
}


void drawCircles() {
    glClear(GL_COLOR_BUFFER_BIT);

    glLineWidth(1.0f);  // Set the line width to 1 pixel

    for (const auto& circle : circles) {
        glBegin(GL_LINE_LOOP);  // Use GL_LINE_LOOP instead of GL_TRIANGLE_FAN
        glColor3ub(circle.color.r, circle.color.g, circle.color.b);
        for (float angle = 0.0f; angle < 2 * M_PI; angle += 0.01f) {
            float x = circle.x + circle.radius * std::cos(angle);
            float y = circle.y + circle.radius * std::sin(angle);
            glVertex2f(x, y);
        }
        glEnd();
    }
    SDL_GL_SwapWindow(window);
}


void handleAutoMode(Uint32 frameDuration, Uint32& delayStartTime, Uint32& noDelayStartTime, Uint32& lastConsoleOutputTime, const Uint32 DELAY_THRESHOLD, const Uint32 NO_DELAY_THRESHOLD, const int originalMaxCircles) {
    if (frameDuration > FRAME_DELAY) {
        if (SDL_GetTicks() - lastConsoleOutputTime >= 2000) {
            if (delayStartTime == 0) {
                delayStartTime = SDL_GetTicks();
            } else if (SDL_GetTicks() - delayStartTime >= DELAY_THRESHOLD) {
                if (MAX_CIRCLES > 10) {
                    MAX_CIRCLES -= 20;
                } else {
                    MAX_CIRCLES = 10;
                }
                std::cout << "Frame rendering falling behind. Lowering MAX_CIRCLES to: " << MAX_CIRCLES << std::endl;
                delayStartTime = 0;  // Reset the delay start time
            }
            noDelayStartTime = 0;  // Reset the no delay start time
            lastConsoleOutputTime = SDL_GetTicks();
        }
    } else {
        delayStartTime = 0;  // Reset the delay start time

        if (noDelayStartTime == 0) {
            noDelayStartTime = SDL_GetTicks();
        } else if (SDL_GetTicks() - noDelayStartTime >= NO_DELAY_THRESHOLD && MAX_CIRCLES < originalMaxCircles) {
            MAX_CIRCLES += 10;
            if (MAX_CIRCLES > originalMaxCircles) {
                MAX_CIRCLES = originalMaxCircles;
            }
            std::cout << "Frame rendering delay improved. Increasing MAX_CIRCLES to: " << MAX_CIRCLES << std::endl;
            noDelayStartTime = 0;  // Reset the no delay start time
        }
        lastConsoleOutputTime = 0;  // Reset the last console output time
    }
}


int main(int argc, char* args[]) {
    initSDL();

    Uint32 frameStartTime, frameEndTime;
    Uint32 lastConsoleOutputTime = 0;

    Uint32 delayStartTime = 0;
    Uint32 noDelayStartTime = 0;


    bool quit = false;
    while (!quit) {
        frameStartTime = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }

        createCircle();
        updateCircles();
        drawCircles();

        frameEndTime = SDL_GetTicks();
        Uint32 frameDuration = frameEndTime - frameStartTime;
        if (frameDuration < FRAME_DELAY) {
            std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_DELAY - frameDuration));
        }

        if (AUTOMODE == 1) {
            handleAutoMode(frameDuration, delayStartTime, noDelayStartTime, lastConsoleOutputTime, DELAY_THRESHOLD, NO_DELAY_THRESHOLD, originalMaxCircles);
        }
    }

    //cleanUp();
    return 0;
}
