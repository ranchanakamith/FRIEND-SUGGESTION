# Friend Suggestion Network

A C++ Data Structures project that models a social network as a graph and recommends friends based on **mutual connections** and **shared interests**. The project includes an interactive GUI built with [Dear ImGui](https://github.com/ocornut/imgui) that visualises the social graph in real time.

---

## What It Does

| Feature | Description |
|---|---|
| Add users | Create a user with any number of interests |
| Add friendships | Connect two users with an undirected edge |
| Get suggestions | Score every non-friend by mutual friends + shared interests |
| Graph view | See all users as nodes and friendships as edges, live |
| Hover tooltip | Mouse over a node to see that person's interests |
| Colour coding | Green = normal user, Cyan = queried user, Gold = suggested friend |

---

## Project Structure

```
FRIEND-SUGGESTION-main/
├── SocialNetwork.h          # Graph data structure (adjacency list)
├── RecommendationEngine.h   # Scoring & suggestion algorithm
├── main_ui.cpp              # GUI entry point (Dear ImGui + GLFW)
├── main.cpp                 # Original CLI entry point (still works)
├── CMakeLists.txt           # Build system (auto-downloads GLFW & ImGui)
├── build.ps1                # One-click build script for Windows
└── build/                   # Created after first build
    └── Release/
        └── FriendSuggestionUI.exe
```

---

## Requirements

### All platforms
- **Git** — for CMake to download dependencies automatically
- **CMake 3.16+**
- **A C++17 compiler** (MSVC 2019+, GCC 9+, or Clang 10+)
- **OpenGL 3.3** capable GPU (any hardware from 2010 onwards)

> Dependencies **GLFW** and **Dear ImGui** are downloaded automatically during the first build. No manual installs needed beyond what is listed above.

---

## Build Instructions

### Windows (Visual Studio 2022) — Recommended

Visual Studio 2022 includes both CMake and a C++ compiler out of the box.

**Step 1 — Install required components**

Open *Visual Studio Installer* and make sure these are checked under your VS 2022 installation:
- `Desktop development with C++`
- `C++ CMake tools for Windows` (inside the above workload)

**Step 2 — Clone or download the project**

```
git clone <repo-url>
cd FRIEND-SUGGESTION-main
```

**Step 3 — Build with the provided script**

Open **PowerShell** in the project folder and run:

```powershell
.\build.ps1
```

The script will:
1. Locate CMake bundled with VS 2022 automatically
2. Download GLFW 3.4 and Dear ImGui v1.91.0 from GitHub (~60 s on first run)
3. Compile everything in Release mode
4. Print the path to the finished `.exe`

**Step 4 — Run**

```powershell
.\build\Release\FriendSuggestionUI.exe
```

---

### Windows (Manual CMake)

If you have CMake installed separately (e.g. from cmake.org):

```powershell
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\FriendSuggestionUI.exe
```

---

### Linux (Ubuntu / Debian)

**Step 1 — Install system packages**

```bash
sudo apt update
sudo apt install cmake build-essential git \
    libgl1-mesa-dev \
    libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev
```

> The X11 headers are required by GLFW to open a window. On Wayland-only systems replace the `libx*` packages with `libwayland-dev`.

**Step 2 — Build**

```bash
cmake -B build -S .
cmake --build build --config Release -j$(nproc)
```

**Step 3 — Run**

```bash
./build/FriendSuggestionUI
```

---

### macOS

**Step 1 — Install tools**

```bash
xcode-select --install      # installs clang and make
brew install cmake git      # requires Homebrew: brew.sh
```

**Step 2 — Build**

```bash
cmake -B build -S .
cmake --build build --config Release
```

**Step 3 — Run**

```bash
./build/FriendSuggestionUI
```

---

## Using the GUI

```
┌──────────────────────────────────────────────────────────────┐
│  Friend Suggestion Network  | 4 users                        │
├──────────────────┬───────────────────────────────────────────┤
│  Add User        │                                           │
│  [Name field ]   │                    Alice                  │
│  [Interest ] [+] │               ●───────────●              │
│  • coding        │              /              \             │
│  [Create User  ] │          Bob ●              ● Carol       │
│                  │              \              /             │
│  Add Friendship  │               ●────────────               │
│  [User A      ]  │             David                         │
│  [User B      ]  │                                           │
│  [Connect     ]  │    (gold nodes = suggested friends)       │
│                  │                                           │
│  Get Suggestions │                                           │
│  [Name        ]  │                                           │
│  [Find Matches]  │                                           │
│                  │                                           │
│  -> David (2)    │                                           │
│     mutual: Bob  │                                           │
│     likes: coding│                                           │
├──────────────────┴───────────────────────────────────────────┤
│  Legend:  ● User   ● Selected   ● Suggested                  │
└──────────────────────────────────────────────────────────────┘
```

### Step-by-step walkthrough

1. **Add users** — Type a name in the Name field. Optionally type an interest and click **+** to queue it (repeat for multiple interests). Click **Create User**.

2. **Connect friends** — Fill in User A and User B, click **Connect**. A line appears between them on the graph.

3. **Get suggestions** — Type a user's name, click **Find Matches**. Suggested friends appear gold in the graph and listed in the panel with their score and reason.

4. **Hover** over any node in the graph to see that user's interests in a tooltip.

---

## How the Code Works

### 1. `SocialNetwork.h` — the Graph

The network is stored as an **adjacency list** using two hash maps:

```
users          : unordered_map<string, User>
adjacencyList  : unordered_map<string, unordered_set<string>>
```

Each `User` holds a name and a `unordered_set<string>` of interests.

```
Alice ──── { Bob, Carol }
Bob   ──── { Alice, David }
Carol ──── { Alice }
David ──── { Bob }
```

Key operations and their complexity:

| Operation | Data structure used | Time complexity |
|---|---|---|
| `addUser` | Insert into two hash maps | O(1) average |
| `addFriendship` | Insert into two sets (both directions) | O(1) average |
| `getFriends` | Hash map lookup | O(1) average |
| `getUser` | Hash map lookup | O(1) average |

Friendships are **undirected**: adding Alice→Bob automatically adds Bob→Alice.

---

### 2. `RecommendationEngine.h` — the Scoring Algorithm

`getSmartSuggestions(network, targetName)` runs in three passes:

**Pass 1 — Filter candidates**

Skip the target user themselves and anyone already a direct friend.

**Pass 2 — Score each candidate**

For every remaining user, count two things:

```
shared_interests = |target.interests ∩ other.interests|
mutual_friends   = |target.friends   ∩ other.friends  |
score            = shared_interests + mutual_friends
```

Finding mutual friends is a set intersection — O(min(|A|, |B|)) per pair.

**Pass 3 — Sort and return**

```cpp
sort by score descending   // O(k log k) where k = number of candidates
```

A user is only included in results if their score > 0 (at least one shared interest or mutual friend).

**Example:**

```
Alice's friends : { Bob }
David's friends : { Bob }

Mutual friends of Alice and David = { Bob }  → score += 1
Shared interests                  = {}       → score += 0
Final score for David             = 1  ✓ suggested
```

---

### 3. `main_ui.cpp` — the GUI Layer

The GUI is built with **Dear ImGui** (immediate-mode UI) rendered via **GLFW** (window + input) and **OpenGL 3.3** (GPU rendering).

#### Render loop (runs ~60 times per second)

```
glfwPollEvents()          ← collect keyboard/mouse events
ImGui::NewFrame()         ← start building the UI for this frame
  BeginChild("controls")  ← left panel: forms and results
  BeginChild("graph")     ← right panel: graph canvas
    refreshLayout()       ← recompute node positions if user count changed
    ImDrawList::AddLine() ← draw each friendship edge
    ImDrawList::AddCircleFilled() ← draw each user node
    ImDrawList::AddText() ← draw name labels
ImGui::Render()           ← bake draw commands
ImGui_ImplOpenGL3_RenderDrawData() ← send to GPU
glfwSwapBuffers()         ← flip front/back buffer → visible on screen
```

#### Graph layout (`refreshLayout`)

Nodes are arranged in a **circular layout**. Positions are recomputed only when the number of users changes or the window is resized, so the graph stays stable between frames.

```
angle[i] = 2π × i / total_users  (sorted alphabetically for determinism)
x[i] = centre_x + radius × cos(angle[i])
y[i] = centre_y + radius × sin(angle[i])
```

#### Node colour coding

```cpp
if (name == queryUser)       → cyan   (the user you searched for)
else if (suggestedSet has name) → gold (a suggested friend)
else                            → green (ordinary user)
```

---

### 4. `CMakeLists.txt` — the Build System

CMake's `FetchContent` module is used to download dependencies automatically:

```
GLFW  → downloaded from github.com/glfw/glfw  (tag 3.4)
ImGui → downloaded from github.com/ocornut/imgui (tag v1.91.0)
```

GLFW is built from source as a static library. ImGui has no CMakeLists.txt of its own, so its `.cpp` files are compiled directly as part of the `FriendSuggestionUI` target. OpenGL is found via the system (`find_package(OpenGL REQUIRED)`).

---

## CLI Version (Original)

The original command-line version still builds and runs independently:

```bash
# Windows (MSVC)
cl /std:c++17 main.cpp /Fe:SocialNetworkCLI.exe

# Linux / macOS
g++ -std=c++17 main.cpp -o SocialNetworkCLI
./SocialNetworkCLI
```

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `cmake` not found on Windows | Open *VS Installer* → modify your install → add *C++ CMake tools for Windows* |
| Configure fails: "Git not found" | Install Git from git-scm.com and restart the terminal |
| Black window / OpenGL errors | Update your GPU drivers; OpenGL 3.3 is required |
| `imgui_impl_opengl3.cpp` errors about `glad` | Ensure you are using the tag `v1.91.0` exactly; newer tags may change the loader |
| Linux: `Cannot find -lGL` | Run `sudo apt install libgl1-mesa-dev` |
| macOS: permission denied running the exe | Run `chmod +x ./build/FriendSuggestionUI` |
