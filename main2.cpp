#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <conio.h>      // _kbhit() and _getch(): read a key without waiting for Enter
#include <windows.h>
using namespace std;

const int WIDTH = 40;
const int HEIGHT = 14;

// ---------- SHARED SETTINGS AND COUNTERS ----------
double mutationRate = 0.2;
int births = 0;
int eaten = 0;
int starved = 0;
int currentTick = 0;

// ---------- EVENT LOG (keeps only the last 3 events) ----------
string events[3] = {"", "", ""};
string lastAction = "(nothing yet)";     // shows the last key you pressed

void addEvent(string text) {
    events[0] = events[1];
    events[1] = events[2];
    events[2] = "[tick " + to_string(currentTick) + "] " + text;
}

// ---------- SMALL HELPER FUNCTIONS ----------
double randomNudge(double rate) {
    return ((rand() % 2001) - 1000) / 1000.0 * rate;
}

double limit(double value, double low, double high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

// ---------- FOOD ----------
struct Food {
    double x, y;
};

// ---------- GLOBAL LISTS (animals read these to look around) ----------
class Animal;
Animal* animals[100];
int animalCount = 0;

Food foods[50];
int foodCount = 0;

// ---------- BASE CLASS ----------
class Animal {
protected:
    double x, y;
    double speed, vision, size;
    double energy;
    bool alive;

public:
    Animal(double x, double y, double speed, double vision, double size) {
        this->x = x;
        this->y = y;
        this->speed = speed;
        this->vision = vision;
        this->size = size;
        energy = 20;
        alive = true;
    }

    double getX() { return x; }
    double getY() { return y; }
    double getSpeed() { return speed; }
    double getVision() { return vision; }
    double getSize() { return size; }
    bool isAlive() { return alive; }
    void die() { alive = false; }

    double distanceTo(double otherX, double otherY) {
        double dx = otherX - x;
        double dy = otherY - y;
        return sqrt(dx * dx + dy * dy);
    }

    void moveToward(double targetX, double targetY, double step) {
        double d = distanceTo(targetX, targetY);
        if (d <= step) {
            x = targetX;
            y = targetY;
        } else {
            x = x + (targetX - x) / d * step;
            y = y + (targetY - y) / d * step;
        }
    }

    void moveAway(double fromX, double fromY, double step) {
        double d = distanceTo(fromX, fromY);
        if (d == 0) return;
        double dx = -(fromX - x) / d * step;
        double dy = -(fromY - y) / d * step;

        if (x + dx < 0 || x + dx > WIDTH - 1) {
            dx = 0;
            dy = (dy >= 0 ? step : -step);
        } else if (y + dy < 0 || y + dy > HEIGHT - 1) {
            dy = 0;
            dx = (dx >= 0 ? step : -step);
        }
        x = x + dx;
        y = y + dy;
    }

    void keepInside() {
        if (x < 0) x = 0;
        if (x > WIDTH - 1) x = WIDTH - 1;
        if (y < 0) y = 0;
        if (y > HEIGHT - 1) y = HEIGHT - 1;
    }

    // living costs energy; if it reaches zero the animal starves
    void payEnergy(double step) {
        energy = energy - (0.3 + 0.3 * size + 0.3 * step * step);
        if (energy <= 0 && alive) {
            alive = false;
            starved++;
            addEvent(string(symbol() == 'r' ? "Rabbit" : "Fox") + " died of starvation");
        }
    }

    void tryReproduce(double threshold) {
        if (alive && energy > threshold && animalCount < 100) {
            Animal* baby = makeBaby();
            energy = energy / 2;
            baby->energy = energy;
            animals[animalCount] = baby;
            animalCount++;
            births++;
            addEvent(string(symbol() == 'r' ? "Rabbit" : "Fox") + " was born");
        }
    }

    virtual void update() = 0;
    virtual char symbol() = 0;
    virtual Animal* makeBaby() = 0;
    virtual ~Animal() {}
};

// ---------- RABBIT ----------
class Rabbit : public Animal {
public:
    Rabbit(double x, double y, double speed, double vision, double size)
        : Animal(x, y, speed, vision, size) {}

    char symbol() { return 'r'; }

    Animal* makeBaby() {
        double s = limit(speed + randomNudge(mutationRate), 0.3, 2.0);
        double v = limit(vision + randomNudge(mutationRate) * 3, 2.0, 10.0);
        double z = limit(size + randomNudge(mutationRate), 0.5, 2.0);
        return new Rabbit(x, y, s, v, z);
    }

    void update() {
        double step = speed;

        // 1. nearest fox inside my vision?
        int foxIndex = -1;
        double best = vision;
        for (int i = 0; i < animalCount; i++) {
            if (animals[i]->symbol() == 'F' && animals[i]->isAlive()) {
                double d = distanceTo(animals[i]->getX(), animals[i]->getY());
                if (d < best) {
                    best = d;
                    foxIndex = i;
                }
            }
        }

        if (foxIndex != -1) {
            // 2. run away, 1.5 times faster
            step = speed * 1.5;
            moveAway(animals[foxIndex]->getX(), animals[foxIndex]->getY(), step);
        } else {
            // 3. nearest food inside my vision?
            int foodIndex = -1;
            best = vision;
            for (int i = 0; i < foodCount; i++) {
                double d = distanceTo(foods[i].x, foods[i].y);
                if (d < best) {
                    best = d;
                    foodIndex = i;
                }
            }

            if (foodIndex != -1) {
                moveToward(foods[foodIndex].x, foods[foodIndex].y, step);
                if (distanceTo(foods[foodIndex].x, foods[foodIndex].y) < 0.5) {
                    energy = energy + 8 + 4 * size;
                    foods[foodIndex] = foods[foodCount - 1];
                    foodCount--;
                }
            } else {
                // 4. nothing to do: wander
                x = x + (rand() % 3 - 1) * step;
                y = y + (rand() % 3 - 1) * step;
            }
        }

        keepInside();
        payEnergy(step);
        tryReproduce(30);
    }
};

// ---------- FOX ----------
class Fox : public Animal {
private:
    int sprintTicks;
    int restTicks;
    double patrolX, patrolY;
    bool hasPatrol;

public:
    Fox(double x, double y, double speed, double vision, double size)
        : Animal(x, y, speed, vision, size) {
        energy = 40;
        sprintTicks = 0;
        restTicks = 0;
        hasPatrol = false;
    }

    char symbol() { return 'F'; }

    Animal* makeBaby() {
        double s = limit(speed + randomNudge(mutationRate), 0.3, 2.0);
        double v = limit(vision + randomNudge(mutationRate) * 3, 2.0, 10.0);
        double z = limit(size + randomNudge(mutationRate), 0.5, 2.0);
        return new Fox(x, y, s, v, z);
    }

    void update() {
        int preyIndex = -1;
        double best = vision;
        for (int i = 0; i < animalCount; i++) {
            if (animals[i]->symbol() == 'r' && animals[i]->isAlive()) {
                double d = distanceTo(animals[i]->getX(), animals[i]->getY());
                if (d < best) {
                    best = d;
                    preyIndex = i;
                }
            }
        }

        double step = speed;

        if (preyIndex != -1) {
            if (restTicks > 0) {
                restTicks--;
                step = speed * 0.5;            // tired
            } else if (best < 3.0) {
                step = speed * 2;              // pounce
                sprintTicks++;
                if (sprintTicks >= 3) {
                    sprintTicks = 0;
                    restTicks = 6;
                }
            }

            moveToward(animals[preyIndex]->getX(), animals[preyIndex]->getY(), step);

            if (distanceTo(animals[preyIndex]->getX(), animals[preyIndex]->getY()) < 0.5) {
                animals[preyIndex]->die();
                energy = energy + 25;
                eaten++;
                addEvent("Fox ate a rabbit");
            }
        } else {
            sprintTicks = 0;
            if (!hasPatrol || distanceTo(patrolX, patrolY) < 1.0) {
                patrolX = rand() % WIDTH;
                patrolY = rand() % HEIGHT;
                hasPatrol = true;
            }
            step = speed * 0.6;
            moveToward(patrolX, patrolY, step);
        }

        keepInside();
        payEnergy(step);
        tryReproduce(70);
    }
};

// ---------- AVERAGE GENES OF ONE KIND OF ANIMAL ----------
int averageGenes(char type, double& s, double& v, double& z) {
    int count = 0;
    s = 0; v = 0; z = 0;
    for (int i = 0; i < animalCount; i++) {
        if (animals[i]->symbol() == type) {
            count++;
            s = s + animals[i]->getSpeed();
            v = v + animals[i]->getVision();
            z = z + animals[i]->getSize();
        }
    }
    if (count > 0) {
        s = s / count;
        v = v / count;
        z = z / count;
    }
    return count;
}

// =====================================================
// THE WORLD CLASS
// =====================================================
class World {
private:
    double startS, startV, startZ;   // rabbits' average genes at the start

    void removeDead() {
        int j = 0;
        for (int i = 0; i < animalCount; i++) {
            if (animals[i]->isAlive()) {
                animals[j] = animals[i];
                j++;
            } else {
                delete animals[i];
            }
        }
        animalCount = j;
    }

public:
    void setup() {
        animals[0] = new Fox(2, 1, 0.8, 5, 1);
        animals[1] = new Fox(5, 3, 0.8, 5, 1);
        animals[2] = new Rabbit(37, 12, 0.8, 4, 1);
        animals[3] = new Rabbit(34, 11, 0.8, 6, 1);
        animals[4] = new Rabbit(38, 9, 0.8, 8, 1);
        animalCount = 5;

        foods[0] = {31, 11};
        foods[1] = {35, 7};
        foods[2] = {30, 8};
        foods[3] = {26, 8};
        foods[4] = {21, 6};
        foodCount = 5;

        averageGenes('r', startS, startV, startZ);
        addEvent("Simulation ready: 2 foxes, 3 rabbits, 5 food");
    }

    // ---- ADMIN ACTIONS ----
    void addRabbits(int n) {
        for (int i = 0; i < n && animalCount < 100; i++) {
            double s = 0.6 + (rand() % 61) / 100.0;
            double v = 4 + rand() % 5;
            double z = 0.8 + (rand() % 41) / 100.0;
            animals[animalCount] = new Rabbit(rand() % WIDTH, rand() % HEIGHT, s, v, z);
            animalCount++;
        }
        addEvent("Admin added " + to_string(n) + " rabbits");
    }

    void addFoxes(int n) {
        for (int i = 0; i < n && animalCount < 100; i++) {
            double s = 0.6 + (rand() % 61) / 100.0;
            double v = 4 + rand() % 5;
            double z = 0.8 + (rand() % 41) / 100.0;
            animals[animalCount] = new Fox(rand() % WIDTH, rand() % HEIGHT, s, v, z);
            animalCount++;
        }
        addEvent("Admin added " + to_string(n) + " foxes");
    }

    void dropFood(double fx, double fy, int n) {
        for (int i = 0; i < n && foodCount < 50; i++) {
            foods[foodCount].x = limit(fx + randomNudge(2), 0, WIDTH - 1);
            foods[foodCount].y = limit(fy + randomNudge(2), 0, HEIGHT - 1);
            foodCount++;
        }
        addEvent("Admin dropped " + to_string(n) + " food");
    }

    void drought() {
        foodCount = 0;
        addEvent("DROUGHT: all food removed");
    }

    void changeMutation(double amount) {
        mutationRate = limit(mutationRate + amount, 0, 1);
        addEvent("Mutation rate is now " + to_string(mutationRate).substr(0, 4));
    }

    // ---- ONE TICK ----
    void step() {
        currentTick++;

        if (rand() % 100 < 10 && foodCount < 50) {
            foods[foodCount].x = rand() % WIDTH;
            foods[foodCount].y = rand() % HEIGHT;
            foodCount++;
        }

        int count = animalCount;
        for (int i = 0; i < count; i++)
            if (animals[i]->isAlive())
                animals[i]->update();

        removeDead();
    }

    int count(char type) {
        int c = 0;
        for (int i = 0; i < animalCount; i++)
            if (animals[i]->symbol() == type) c++;
        return c;
    }

    // ---- DRAW (every line ends with \033[K to erase leftovers) ----
    void draw(bool running, int delayMs) {
        char grid[HEIGHT][WIDTH];
        for (int i = 0; i < HEIGHT; i++)
            for (int j = 0; j < WIDTH; j++)
                grid[i][j] = '.';

        for (int i = 0; i < foodCount; i++)
            grid[(int)foods[i].y][(int)foods[i].x] = '*';

        for (int i = 0; i < animalCount; i++)
            grid[(int)animals[i]->getY()][(int)animals[i]->getX()] = animals[i]->symbol();

        cout << "=== BIO-DOME  Tick " << currentTick << "  "
             << (running ? "[RUNNING]" : "[PAUSED]") << " ===\033[K\n";
        cout << "r = rabbit   F = fox   * = food\033[K\n";
        for (int i = 0; i < HEIGHT; i++) {
            for (int j = 0; j < WIDTH; j++)
                cout << grid[i][j];
            cout << "\033[K\n";
        }

        int rabbits = count('r');
        cout << "Rabbits: " << rabbits << "  Foxes: " << count('F')
             << "  Food: " << foodCount << "\033[K\n";
        cout << "Births: " << births << "  Eaten: " << eaten
             << "  Starved: " << starved << "\033[K\n";
        cout << "Mutation rate: " << fixed << setprecision(2) << mutationRate
             << "   Delay: " << delayMs << " ms\033[K\n";
        cout << "Last key: " << lastAction << "\033[K\n";

        double nowS, nowV, nowZ;
        if (averageGenes('r', nowS, nowV, nowZ) > 0) {
            cout << "Rabbit genes   start    now\033[K\n";
            cout << "  speed       " << setw(6) << startS << " " << setw(6) << nowS << "\033[K\n";
            cout << "  vision      " << setw(6) << startV << " " << setw(6) << nowV << "\033[K\n";
            cout << "  size        " << setw(6) << startZ << " " << setw(6) << nowZ << "\033[K\n";
        } else {
            cout << "NO RABBITS ALIVE - press R to add some\033[K\n\033[K\n\033[K\n\033[K\n";
        }

        cout << "Recent events:\033[K\n";
        for (int i = 0; i < 3; i++)
            cout << "  " << events[i] << "\033[K\n";
    }

    void cleanup() {
        for (int i = 0; i < animalCount; i++) delete animals[i];
        animalCount = 0;
    }
};

// ---------- MAIN ----------
int main() {
    srand(time(0));

    // allow colors / cursor codes in the Windows console
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(h, &mode);
    SetConsoleMode(h, mode | 0x0004);

    World world;
    world.setup();

    bool running = false;      // starts paused so you can explain first
    bool quit = false;
    int delayMs = 300;

    cout << "\033[2J\033[?25l";    // clear screen, hide cursor

    while (!quit) {
        // ---- 1. DRAW: build the frame in memory, then print it once ----
        ostringstream frame;
        streambuf* old = cout.rdbuf(frame.rdbuf());
        world.draw(running, delayMs);
        cout << "CONTROLS\033[K\n";
        cout << " [SPACE] run/pause  [N] one step  [1-5] speed (slow..fast)\033[K\n";
        cout << " [R] +5 rabbits  [F] +2 foxes  [D] drop food  [K] drought\033[K\n";
        cout << " [+/-] mutation rate  [Q] quit\033[K\n";
        cout << "\033[J";
        cout.rdbuf(old);

        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(h, &info);
        SetConsoleCursorPosition(h, COORD{0, info.srWindow.Top});
        cout << frame.str();
        cout.flush();

        // ---- 2. WAIT / READ A KEY ----
        char key = 0;
        if (running) {
            this_thread::sleep_for(chrono::milliseconds(delayMs));
            if (_kbhit()) key = _getch();       // a key was pressed during the wait
        } else {
            key = _getch();                     // paused: wait here until a key is pressed
        }

        // ---- 3. HANDLE THE KEY (and remember it in lastAction) ----
        bool stepOnce = false;
        if (key == 'q' || key == 'Q') {
            quit = true;
            lastAction = "[Q] quitting";
        } else if (key == ' ') {
            running = !running;
            lastAction = running ? "[SPACE] simulation RUNNING" : "[SPACE] simulation PAUSED";
        } else if (key == 'n' || key == 'N') {
            stepOnce = true;
            lastAction = "[N] one step";
        } else if (key == '1') { delayMs = 800; lastAction = "[1] speed: very slow"; }
        else if (key == '2') { delayMs = 400; lastAction = "[2] speed: slow"; }
        else if (key == '3') { delayMs = 200; lastAction = "[3] speed: medium"; }
        else if (key == '4') { delayMs = 80;  lastAction = "[4] speed: fast"; }
        else if (key == '5') { delayMs = 20;  lastAction = "[5] speed: very fast"; }
        else if (key == 'r' || key == 'R') {
            world.addRabbits(5);
            lastAction = "[R] added 5 rabbits";
        } else if (key == 'f' || key == 'F') {
            world.addFoxes(2);
            lastAction = "[F] added 2 foxes";
        } else if (key == 'k' || key == 'K') {
            world.drought();
            lastAction = "[K] drought, all food removed";
        } else if (key == '+') {
            world.changeMutation(0.05);
            lastAction = "[+] mutation rate increased";
        } else if (key == '-') {
            world.changeMutation(-0.05);
            lastAction = "[-] mutation rate decreased";
        } else if (key == 'd' || key == 'D') {
            cout << "\033[?25h\n";               // show the cursor while typing
            string a;
            cout << "Food x (0-39): ";  getline(cin, a);  double fx = atof(a.c_str());
            cout << "Food y (0-13): ";  getline(cin, a);  double fy = atof(a.c_str());
            cout << "How many: ";       getline(cin, a);  int n = atoi(a.c_str());
            world.dropFood(fx, fy, n);
            lastAction = "[D] dropped " + to_string(n) + " food near ("
                         + to_string((int)fx) + ", " + to_string((int)fy) + ")";
            cout << "\033[2J\033[?25l";          // clear screen again
        } else if (key != 0) {
            lastAction = string("'") + key + "' is not a command";
        }

        // ---- 4. UPDATE ----
        if (running || stepOnce) world.step();
    }

    cout << "\033[?25h\n";
    world.cleanup();
    return 0;
}