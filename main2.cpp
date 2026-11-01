
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
using namespace std;

// =====================================================
// SIMULATION LOGIC ONLY (no window, no drawing)
// =====================================================
const int WIDTH = 40;
const int HEIGHT = 14;
const int MAX_FOOD = 100;

double mutationRate = 0.2;
int births = 0;
int eaten = 0;
int starved = 0;
int currentTick = 0;

// the last 3 events
string events[3] = {"", "", ""};
string lastAction = "(nothing yet)";

void addEvent(string text) {
    events[0] = events[1];
    events[1] = events[2];
    events[2] = "[tick " + to_string(currentTick) + "] " + text;
}

// random number between -rate and +rate
double randomNudge(double rate) {
    return ((rand() % 2001) - 1000) / 1000.0 * rate;
}

// keep a value between low and high
double limit(double value, double low, double high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

struct Food {
    double x, y;
};

class Animal;
Animal* animals[100];
int animalCount = 0;

Food foods[MAX_FOOD];
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
        energy = 40;
        alive = true;
    }

    double getX() { return x; }
    double getY() { return y; }
    double getSpeed() { return speed; }
    double getVision() { return vision; }
    double getSize() { return size; }
    double getEnergy() { return energy; }
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

    void payEnergy(double step) {
        energy = energy - (0.1 + 0.1 * size + 0.1 * step * step);
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
            step = speed * 1.5;
            moveAway(animals[foxIndex]->getX(), animals[foxIndex]->getY(), step);
        } else {
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
                    energy = energy + 15 + 5 * size;
                    foods[foodIndex] = foods[foodCount - 1];
                    foodCount--;
                }
            } else {
                x = x + (rand() % 3 - 1) * step;
                y = y + (rand() % 3 - 1) * step;
            }
        }

        keepInside();
        payEnergy(step);
        tryReproduce(70);
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
        energy = 100;
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
                step = speed * 0.5;
            } else if (best < 3.0) {
                step = speed * 2;
                sprintTicks++;
                if (sprintTicks >= 3) {
                    sprintTicks = 0;
                    restTicks = 6;
                }
            }

            moveToward(animals[preyIndex]->getX(), animals[preyIndex]->getY(), step);

            if (distanceTo(animals[preyIndex]->getX(), animals[preyIndex]->getY()) < 0.5) {
                animals[preyIndex]->die();
                energy = energy + 40;
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
        tryReproduce(160);
    }
};

// average genes of one kind of animal ('r' or 'F'); returns how many there are
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

// ---------- THE WORLD ----------
class World {
private:
    double startS, startV, startZ;

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

        for (int i = 0; i < 10; i++) {
            foods[foodCount] = { (double)(rand() % WIDTH), (double)(rand() % HEIGHT) };
            foodCount++;
        }

        averageGenes('r', startS, startV, startZ);
        addEvent("Simulation ready: 2 foxes, 3 rabbits, 15 food");
    }

    // the rabbits' average genes at the start (the window shows them)
    void getStart(double& s, double& v, double& z) {
        s = startS; v = startV; z = startZ;
    }

    // ---- admin actions ----
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
        for (int i = 0; i < n && foodCount < MAX_FOOD; i++) {
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

    // ---- one tick of the simulation ----
    void step() {
        currentTick++;

        if (rand() % 100 < 30 && foodCount < MAX_FOOD) {
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

    void cleanup() {
        for (int i = 0; i < animalCount; i++) delete animals[i];
        animalCount = 0;
    }
};