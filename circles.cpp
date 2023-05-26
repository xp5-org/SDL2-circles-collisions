#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <random>
#include <cmath>
#include <chrono>
#include <thread>
#include <deque>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int CIRCLE_MIN_RADIUS = 8;
const int CIRCLE_MAX_RADIUS = 64;

const float CIRCLE_SPEED = 0.5;
const int MAX_CIRCLES = 20;  // Maximum number of circles
const int FRAMES_PER_SECOND = 30;
const int FRAME_DELAY = 1000 / FRAMES_PER_SECOND;
const int SPAWN_RATE = 600; // milliseconds to wait between spawning

struct Circle {
    float x;
    float y;
    float radius;
    SDL_Color color;
    float dx;
    float dy;
};

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
std::deque<Circle> circles;
SDL_Rect leftWall;
SDL_Rect rightWall;

void initSDL() {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Circle Collision", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
   SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
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
    
    if (currentTime - lastCircleCreationTime >= SPAWN_RATE) {
        if (circles.size() >= MAX_CIRCLES) {
            circles.pop_front();  // Remove the oldest circle
        }
        
        Circle circle = createRandomCircle();
        circles.push_back(circle);
        
        lastCircleCreationTime = currentTime;  // Update the last creation time
    }
}


void createWalls() {
    //leftWall.x = 0;
    //leftWall.y = SCREEN_HEIGHT - WALL_HEIGHT;
    //leftWall.w = (SCREEN_WIDTH - WALL_GAP) / 2;
    //leftWall.h = WALL_HEIGHT;

    //rightWall.x = leftWall.x + leftWall.w + WALL_GAP;
    //rightWall.y = SCREEN_HEIGHT - WALL_HEIGHT;
    //rightWall.w = (SCREEN_WIDTH - WALL_GAP) / 2;
    //rightWall.h = WALL_HEIGHT;
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
    float overlap = 0.5f * (distance - circle1.radius - circle2.radius);

    circle1.x += overlap * (dx / distance);
    circle1.y += overlap * (dy / distance);

    circle2.x -= overlap * (dx / distance);
    circle2.y -= overlap * (dy / distance);
}

void updateCircles() {
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
}

void drawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    int x = radius;
    int y = 0;
    int radiusError = 1 - x;

    while (x >= y) {
        for (int i = -1; i <= 1; i += 2) {
            SDL_RenderDrawPoint(renderer, centerX + x * i, centerY + y);
            SDL_RenderDrawPoint(renderer, centerX + y * i, centerY + x);
            SDL_RenderDrawPoint(renderer, centerX + x * i, centerY - y);
            SDL_RenderDrawPoint(renderer, centerX + y * i, centerY - x);
        }
        y++;

        if (radiusError < 0) {
            radiusError += 2 * y + 1;
        } else {
            x--;
            radiusError += 2 * (y - x) + 1;
        }
    }
}



void drawCircles() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (const auto& circle : circles) {
        SDL_SetRenderDrawColor(renderer, circle.color.r, circle.color.g, circle.color.b, 255);
        drawCircle(renderer, static_cast<int>(circle.x), static_cast<int>(circle.y), static_cast<int>(circle.radius));
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &leftWall);
    SDL_RenderFillRect(renderer, &rightWall);

    SDL_RenderPresent(renderer);
}

void cleanUp() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char* args[]) {
    initSDL();
    createWalls();

    Uint32 frameStartTime, frameEndTime;

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
    }

    cleanUp();
    return 0;
}
