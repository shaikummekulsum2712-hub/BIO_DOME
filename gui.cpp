#include "raylib.h"
#include "main2.cpp"      // brings in all the simulation logic from main.cpp

// =====================================================
// THE GUI: draws the window and reads buttons and clicks
// =====================================================
const int SCREEN_W = 940;
const int SCREEN_H = 700;
const int MAPX = 30;
const int MAPY = 50;
const int CELL = 22;       // one world cell = 22 x 22 pixels

int pixelX(double x) { return MAPX + (int)(x * CELL) + CELL / 2; }
int pixelY(double y) { return MAPY + (int)(y * CELL) + CELL / 2; }

Color PANEL = {18, 55, 42, 255};

// draws a button; returns true on the frame it is clicked
bool button(int x, int y, int w, int h, const char* label, Color base) {
    Rectangle r = {(float)x, (float)y, (float)w, (float)h};
    bool hover = CheckCollisionPointRec(GetMousePosition(), r);
    DrawRectangleRounded(r, 0.3f, 8, hover ? ColorBrightness(base, 0.25f) : base);
    int tw = MeasureText(label, 18);
    DrawText(label, x + (w - tw) / 2, y + (h - 18) / 2, 18, WHITE);
    return hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void drawMap(bool showVision, bool noRabbits) {
    DrawRectangle(MAPX, MAPY, WIDTH * CELL, HEIGHT * CELL, Color{26, 71, 49, 255});

    for (int i = 0; i <= WIDTH; i++)
        DrawLine(MAPX + i * CELL, MAPY, MAPX + i * CELL, MAPY + HEIGHT * CELL, Color{255, 255, 255, 15});
    for (int j = 0; j <= HEIGHT; j++)
        DrawLine(MAPX, MAPY + j * CELL, MAPX + WIDTH * CELL, MAPY + j * CELL, Color{255, 255, 255, 15});

    // food = yellow dots
    for (int i = 0; i < foodCount; i++)
        DrawCircle(pixelX(foods[i].x), pixelY(foods[i].y), 4, GOLD);

    for (int i = 0; i < animalCount; i++) {
        Animal* a = animals[i];
        int x = pixelX(a->getX());
        int y = pixelY(a->getY());
        bool isRabbit = (a->symbol() == 'r');

        // vision ring: how far this animal can see
        if (showVision)
            DrawCircleLines(x, y, (float)(a->getVision() * CELL),
                            isRabbit ? Color{150, 255, 170, 120} : Color{255, 140, 100, 120});

        if (isRabbit) {
            // rabbit = white circle with two ears (bigger size gene = bigger body)
            int r = (int)(5 + a->getSize() * 3);
            DrawCircle(x - 3, y - r - 1, 3, LIGHTGRAY);
            DrawCircle(x + 3, y - r - 1, 3, LIGHTGRAY);
            DrawCircle(x, y, (float)r, Color{240, 240, 240, 255});
        } else {
            // fox = orange triangle
            float r = 7 + (float)a->getSize() * 4;
            DrawPoly(Vector2{(float)x, (float)y}, 3, r, 270, ORANGE);
            DrawPolyLines(Vector2{(float)x, (float)y}, 3, r, 270, MAROON);
        }

        // energy bar above the animal
        double maxEnergy = isRabbit ? 70.0 : 160.0;
        double f = limit(a->getEnergy() / maxEnergy, 0, 1);
        DrawRectangle(x - 8, y - 20, 16, 3, DARKGRAY);
        DrawRectangle(x - 8, y - 20, (int)(16 * f), 3, f > 0.3 ? LIME : RED);
    }

    if (noRabbits) {
        const char* msg = "NO RABBITS ALIVE - press R to add some";
        int tw = MeasureText(msg, 26);
        DrawText(msg, MAPX + (WIDTH * CELL - tw) / 2, MAPY + HEIGHT * CELL / 2 - 13, 26, RED);
    }
}

void drawPanels(World& world, int speedLevel) {
    int y = 465, h = 215;
    DrawRectangleRounded(Rectangle{30, (float)y, 250, (float)h}, 0.06f, 8, PANEL);
    DrawRectangleRounded(Rectangle{295, (float)y, 290, (float)h}, 0.06f, 8, PANEL);
    DrawRectangleRounded(Rectangle{600, (float)y, 310, (float)h}, 0.06f, 8, PANEL);

    // left panel: population and counters
    DrawText("Population", 45, y + 10, 20, GOLD);
    int ly = y + 40;
    DrawText(TextFormat("Rabbits: %d", world.count('r')), 45, ly, 16, RAYWHITE); ly += 21;
    DrawText(TextFormat("Foxes: %d", world.count('F')), 45, ly, 16, RAYWHITE); ly += 21;
    DrawText(TextFormat("Food: %d", foodCount), 45, ly, 16, RAYWHITE); ly += 21;
    DrawText(TextFormat("Births: %d", births), 45, ly, 16, RAYWHITE); ly += 21;
    DrawText(TextFormat("Eaten by foxes: %d", eaten), 45, ly, 16, RAYWHITE); ly += 21;
    DrawText(TextFormat("Starved: %d", starved), 45, ly, 16, RAYWHITE); ly += 21;
    DrawText(TextFormat("Mutation rate: %.2f", mutationRate), 45, ly, 16, RAYWHITE); ly += 21;
    DrawText(TextFormat("Speed: %d / 5", speedLevel + 1), 45, ly, 16, RAYWHITE);

    // middle panel: gene table (this is where evolution shows)
    DrawText("Rabbit genes (average)", 310, y + 10, 20, GOLD);
    DrawText("gene", 310, y + 42, 15, LIGHTGRAY);
    DrawText("start", 385, y + 42, 15, LIGHTGRAY);
    DrawText("now", 445, y + 42, 15, LIGHTGRAY);
    DrawText("change", 500, y + 42, 15, LIGHTGRAY);

    double s0, v0, z0, s1, v1, z1;
    world.getStart(s0, v0, z0);
    if (averageGenes('r', s1, v1, z1) > 0) {
        double now[3] = {s1, v1, z1}, start[3] = {s0, v0, z0};
        const char* names[3] = {"speed", "vision", "size"};
        for (int i = 0; i < 3; i++) {
            int ry = y + 68 + i * 26;
            double change = now[i] - start[i];
            DrawText(names[i], 310, ry, 17, RAYWHITE);
            DrawText(TextFormat("%.2f", start[i]), 385, ry, 17, RAYWHITE);
            DrawText(TextFormat("%.2f", now[i]), 445, ry, 17, RAYWHITE);
            DrawText(TextFormat("%+.2f", change), 500, ry, 17, change >= 0 ? LIME : ORANGE);
        }
    } else {
        DrawText("(no rabbits alive)", 310, y + 70, 17, RED);
    }

    double fs, fv, fz;
    if (averageGenes('F', fs, fv, fz) > 0)
        DrawText(TextFormat("Fox avg: speed %.2f  vision %.2f", fs, fv), 310, y + 160, 15, Color{255, 170, 120, 255});

    // right panel: recent events and last action
    DrawText("Recent events", 615, y + 10, 20, GOLD);
    for (int i = 0; i < 3; i++)
        DrawText(events[i].c_str(), 615, y + 45 + i * 24, 14, RAYWHITE);
    DrawText("Last action:", 615, y + 130, 16, LIGHTGRAY);
    DrawText(lastAction.c_str(), 615, y + 152, 14, YELLOW);
}

// =====================================================
// MAIN: opens the window and runs the loop
// =====================================================
int main() {
    srand(time(0));
    InitWindow(SCREEN_W, SCREEN_H, "Bio-Dome Ecosystem Simulator");
    SetTargetFPS(60);

    World world;
    world.setup();

    bool running = false;                      // starts paused
    bool showVision = false;
    int delays[5] = {800, 400, 200, 80, 20};   // milliseconds between ticks
    int speedLevel = 1;
    float timer = 0;

    while (!WindowShouldClose()) {             // Esc or the X button closes the window
        // ----- 1. keys -----
        bool doPause = IsKeyPressed(KEY_SPACE);
        bool doStep = IsKeyPressed(KEY_N);
        bool doSlower = false, doFaster = false;
        bool doRabbits = IsKeyPressed(KEY_R);
        bool doFoxes = IsKeyPressed(KEY_F);
        bool doDrought = IsKeyPressed(KEY_K);
        bool doVision = IsKeyPressed(KEY_V);
        bool doMutUp = IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD);
        bool doMutDown = IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT);

        // click on the map = drop 5 food there
        Vector2 m = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            m.x >= MAPX && m.x < MAPX + WIDTH * CELL &&
            m.y >= MAPY && m.y < MAPY + HEIGHT * CELL) {
            double fx = (m.x - MAPX) / CELL;
            double fy = (m.y - MAPY) / CELL;
            world.dropFood(fx, fy, 5);
            lastAction = "[Click] dropped 5 food at (" + to_string((int)fx) + ", " + to_string((int)fy) + ")";
        }

        // ----- 2. draw everything -----
        BeginDrawing();
        ClearBackground(Color{12, 40, 30, 255});

        DrawText("BIO-DOME  Ecosystem Simulator", 30, 12, 28, RAYWHITE);
        DrawText(TextFormat("Tick %d", currentTick), 640, 16, 22, LIGHTGRAY);
        DrawText(running ? "RUNNING" : "PAUSED", 760, 16, 22, running ? LIME : ORANGE);

        drawMap(showVision, world.count('r') == 0);

        // button row 1
        int bx = 30, by = 372;
        if (button(bx, by, 120, 36, running ? "Pause" : "Run", Color{46, 125, 79, 255})) doPause = true;
        bx += 126;
        if (button(bx, by, 80, 36, "Step", Color{76, 106, 146, 255})) doStep = true;
        bx += 86;
        if (button(bx, by, 100, 36, "Slower", Color{76, 106, 146, 255})) doSlower = true;
        bx += 106;
        if (button(bx, by, 100, 36, "Faster", Color{76, 106, 146, 255})) doFaster = true;
        bx += 106;
        if (button(bx, by, 120, 36, "+5 Rabbits", Color{46, 125, 79, 255})) doRabbits = true;
        bx += 126;
        if (button(bx, by, 110, 36, "+2 Foxes", Color{200, 80, 40, 255})) doFoxes = true;
        bx += 116;
        if (button(bx, by, 100, 36, "Drought", Color{122, 78, 45, 255})) doDrought = true;
        bx += 106;
        if (button(bx, by, 100, 36, showVision ? "Vision:ON" : "Vision:OFF", Color{90, 90, 120, 255})) doVision = true;

        // button row 2
        if (button(30, 416, 120, 36, "Mutation -", Color{90, 90, 120, 255})) doMutDown = true;
        if (button(156, 416, 120, 36, "Mutation +", Color{90, 90, 120, 255})) doMutUp = true;
        DrawText("Click on the map to drop food.  Keys: SPACE N R F K V + -", 295, 426, 18, LIGHTGRAY);

        drawPanels(world, speedLevel);
        EndDrawing();

        // ----- 3. do what the user asked -----
        if (doPause) {
            running = !running;
            lastAction = running ? "simulation RUNNING" : "simulation PAUSED";
        }
        if (doStep) { world.step(); lastAction = "one step"; }
        if (doSlower && speedLevel > 0) { speedLevel--; lastAction = "speed slower"; }
        if (doFaster && speedLevel < 4) { speedLevel++; lastAction = "speed faster"; }
        if (doRabbits) { world.addRabbits(5); lastAction = "added 5 rabbits"; }
        if (doFoxes) { world.addFoxes(2); lastAction = "added 2 foxes"; }
        if (doDrought) { world.drought(); lastAction = "drought: all food removed"; }
        if (doVision) { showVision = !showVision; lastAction = showVision ? "vision rings ON" : "vision rings OFF"; }
        if (doMutUp) { world.changeMutation(0.05); lastAction = "mutation rate up"; }
        if (doMutDown) { world.changeMutation(-0.05); lastAction = "mutation rate down"; }

        // ----- 4. move the simulation forward with time -----
        if (running) {
            timer += GetFrameTime() * 1000;
            if (timer >= delays[speedLevel]) {
                world.step();
                timer = 0;
            }
        }
    }

    world.cleanup();
    CloseWindow();
    return 0;
}