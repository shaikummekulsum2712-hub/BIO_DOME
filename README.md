# 🐇🦊 Bio-Dome: Interactive Ecosystem Simulator

A real-time ecosystem simulator in C++ that runs in the terminal. Rabbits look for food and run from foxes, and foxes patrol, chase and pounce. Every animal has genes that are passed to its babies with small random mutations, so the population evolves over time. While it runs, you act as the "administrator" and change the environment live.


## Features

- **Rabbits** look for the nearest food, and flee (at 1.5x speed) when a fox is in their vision
- **Foxes** patrol, chase rabbits, and pounce (a short sprint followed by a rest)
- **Genes:** speed, vision and size, copied to babies with random mutation
- **Energy system:** moving, size and speed cost energy, eating restores it, and zero energy means starvation
- **Live admin controls:** drop food, add animals, cause a drought, change the mutation rate
- **Live stats:** population, births, kills, starvation, average rabbit genes (start vs now), and the 3 most recent events

## How evolution happens

Evolution is never programmed directly. Faster or far-sighted rabbits survive longer and reproduce more, so the average genes shown in the stats table drift over time. Every gene has an energy cost, which stops traits from simply rising to the maximum.

## OOP concepts used

| Concept | Where |
|---|---|
| Abstraction | `Animal` is an abstract base class (pure virtual `update`, `symbol`, `makeBaby`) |
| Inheritance | `Rabbit` and `Fox` inherit from `Animal` |
| Polymorphism | `animals[i]->update()` runs different behavior for each species |
| Encapsulation | Protected data with getters; Fox-only state is private |
| Composition | `World` manages the animals and food |
| Dynamic memory | `new` / `delete`, with a virtual destructor |

## Build and run

Requires a C++ compiler (e.g. MinGW-w64 / g++). **Windows only**, because it uses `conio.h` and `windows.h` for key input.

```
g++ -std=c++17 main.cpp -o biodome.exe
./biodome.exe
```

Make the terminal window tall (about 32 rows) so the whole screen fits.

## Controls

| Key | Action |
|---|---|
| `SPACE` | Run / pause |
| `N` | Advance one tick (when paused) |
| `1` – `5` | Simulation speed (slow to fast) |
| `R` | Add 5 rabbits |
| `F` | Add 2 foxes |
| `D` | Drop food at a chosen x, y |
| `K` | Drought (remove all food) |
| `+` / `-` | Raise / lower the mutation rate |
| `Q` | Quit |

## Project structure

```
BioDome/
├── main.cpp     # Animal, Rabbit, Fox, World classes and the main loop
└── README.md
```

## Limitations and future work

- Babies copy one parent's genes (no crossover between two parents yet)
- Animals read global arrays; a cleaner design would keep them inside `World`
- Planned: weather effects, save/load, CSV export of gene averages, a graphical interface

## Author

[Your name] · [LinkedIn link]
