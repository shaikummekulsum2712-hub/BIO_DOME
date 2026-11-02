# 🐇🦊 Bio-Dome: Interactive Ecosystem Simulator

A real-time ecosystem simulator written in C++ with a raylib window. Rabbits look for food and run from foxes, and foxes patrol, chase and pounce. Every animal has three genes (speed, vision, size) that are copied to its babies with small random mutations, so the population evolves on its own. You are the "administrator": drop food, add animals, cause a drought, or change the mutation rate while it runs.


## Features

- **Rabbits** look for the nearest food and flee at 1.5x speed when a fox is inside their vision
- **Foxes** patrol, chase the nearest rabbit, and pounce (3 ticks of double speed, then 6 ticks of rest)
- **Genes:** speed, vision and size, passed to babies with random mutation
- **Energy system:** living, size and speed cost energy; eating restores it; zero energy means starvation
- **Live admin controls:** click the map to drop food, add animals, drought, mutation rate, speed
- **Live statistics:** population, births, kills, starvation, average rabbit genes (start vs now), recent events
- **Visual aids:** energy bar above each animal and optional vision rings

## How evolution happens

Evolution is never programmed directly. Babies get slightly mutated genes, every gene costs energy, and foxes and hunger remove the animals that cope worst. After a few hundred ticks, the average genes in the stats table drift. No line of code says "make rabbits faster".

## OOP concepts used

| Concept | Where |
|---|---|
| Abstraction | `Animal` is abstract (pure virtual `update`, `symbol`, `makeBaby`) |
| Inheritance | `Rabbit` and `Fox` inherit from `Animal` |
| Polymorphism | `animals[i]->update()` runs different behaviour per species |
| Encapsulation | `protected` data with getters; fox-only state is `private` |
| Composition | `World` manages the animals and food |
| Dynamic memory | `new` / `delete` with a virtual destructor |
| References | `averageGenes(char, double&, double&, double&)` returns several values |

## Project structure

```
BIODOME/
├── main.cpp           # Simulation logic: Animal, Rabbit, Fox, World (no drawing)
├── gui.cpp            # raylib window: drawing, buttons, main() (includes main.cpp)
├── main_console.cpp   # Optional: the older all-in-one terminal version
└── README.md
```

`gui.cpp` contains `#include "main.cpp"`, so you compile **only `gui.cpp`**. Do not list `main.cpp` in the compile command, or you will get "defined twice" errors.

## Requirements

- Windows 10 or 11
- MSYS2 with the UCRT64 toolchain (g++) and raylib

## Setup (one time)

1. Install MSYS2 (skip if `C:\msys64` already exists):
   ```powershell
   winget install MSYS2.MSYS2
   ```
2. Install the compiler and raylib. `pacman` is **not** a PowerShell command, so call it with its full path:
   ```powershell
   C:\msys64\usr\bin\pacman.exe -S mingw-w64-ucrt-x86_64-gcc
   C:\msys64\usr\bin\pacman.exe -S mingw-w64-ucrt-x86_64-raylib
   ```
   Type `Y` and press Enter when asked.
3. Check that raylib is installed (should print `True`):
   ```powershell
   Test-Path C:\msys64\ucrt64\include\raylib.h
   ```

## Build and run (every time)

Open PowerShell, go to the project folder (the one that contains `gui.cpp`), and run:

```powershell
cd "C:\Users\shaik\OneDrive\Desktop\genetic ecosystem\BIODOME"
dir *.cpp
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
g++ -std=c++17 gui.cpp -o biodome_gui.exe -lraylib -lopengl32 -lgdi32 -lwinmm; if ($?) { .\biodome_gui.exe }
```

- `dir *.cpp` should list both `main.cpp` and `gui.cpp`.
- The `$env:Path = ...` line has to be repeated in every new PowerShell window.
- `if ($?) { ... }` runs the program only when compiling succeeded, so you never run an old `.exe` by accident.

### Optional: the terminal version

```powershell
g++ -std=c++17 main_console.cpp -o biodome.exe; if ($?) { .\biodome.exe }
```

Make the PowerShell window tall (about 35 rows). Keys: `SPACE` run/pause, `N` step, `1`-`5` speed, `R` rabbits, `F` foxes, `D` drop food, `K` drought, `+`/`-` mutation, `Q` quit.

## Controls (GUI)

The window opens **paused**. Press **Run** or `SPACE` to start.

| Control | Action |
|---|---|
| `SPACE` / **Run, Pause** button | Run or pause |
| `N` / **Step** | Advance one tick |
| **Slower / Faster** | Change simulation speed (5 levels) |
| **Left-click on the map** | Drop 5 food where you click |
| `R` / **+5 Rabbits** | Add 5 rabbits |
| `F` / **+2 Foxes** | Add 2 foxes |
| `K` / **Drought** | Remove all food |
| `V` / **Vision** | Show or hide vision rings |
| `+` / `-` or **Mutation +/-** | Raise or lower the mutation rate |
| `Esc` or close button | Quit |

## Suggested demo

1. Start paused and point out the rabbits (white), foxes (orange triangles) and food (yellow dots).
2. Press **Vision** to show how far each animal can see.
3. Press **Step** a few times, then **Run**, and watch rabbits head for food and flee foxes.
4. Click near the foxes to drop bait and watch rabbits take the risk.
5. Press **Drought**, then **+2 Foxes**, then raise the mutation rate, and watch the population and the gene table react.

## Tuning the balance

| Problem | Change in `main.cpp` |
|---|---|
| Animals starve too fast | Lower the three `0.1` values in `payEnergy` |
| Rabbits multiply too much | Raise `tryReproduce(70)` in `Rabbit`, or lower the grass chance (`rand() % 100 < 30`) |
| Foxes eat everything | Raise `restTicks = 6` in `Fox`, or lower fox vision in `setup()` |
| Nobody ever dies | Raise the `0.1` values in `payEnergy` |

## Troubleshooting

| Message | Fix |
|---|---|
| `pacman : The term 'pacman' is not recognized` | Use `C:\msys64\usr\bin\pacman.exe ...`, or open the **MSYS2 UCRT64** window |
| `fatal error: gui.cpp: No such file or directory` | You are in the wrong folder. `cd` into `BIODOME` and check with `dir *.cpp` |
| `raylib.h: No such file or directory` / `cannot find -lraylib` | Install raylib (Setup, step 2) |
| `multiple definition of ...` | You compiled `main.cpp` as well. Compile only `gui.cpp` |
| Old behaviour after editing | Compile failed, so the old `.exe` ran. Read the `error:` lines |
| Window opens and closes at once | Run it from PowerShell so the error stays visible |

## Limitations and future work

- Babies copy one parent's genes (no crossover between two parents yet)
- Animals read global arrays; a cleaner design would keep them inside `World`
- Planned: weather effects, save/load, CSV export of gene averages, separate `.h` files

## Upload to GitHub (optional)

```powershell
git init
echo "*.exe" > .gitignore
git add .
git commit -m "Bio-Dome ecosystem simulator"
git branch -M main
git remote add origin https://github.com/<your-username>/biodome.git
git push -u origin main
```

## Author

[Your name] · [LinkedIn link]
