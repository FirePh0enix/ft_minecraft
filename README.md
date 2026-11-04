# ft_minecraft

## Gameplay

[![Gameplay](https://img.youtube.com/vi/cGg8mxncOXg/0.jpg)](https://youtu.be/cGg8mxncOXg)

## Table of Contents

1. [Description](#description)
2. [Features](#features)
3. [Controls](#controls)
4. [Commands](#commands)
5. [Technologies used](#technologies-used)
6. [Requirements](#requirements)
7. [Installation](#installation)
8. [Run](#run)

## Description

ft_minecraft is a project from the 42 Post-Common Core curriculum, designed to introduce students to advanced procedural generation, graphics programming, and multiplayer networking using low-level graphics APIs.

### Purpose

The objective is to create a Minecraft-like voxel world with diverse biomes, caves, vegetation, and multiplayer exploration.

## Features

Features are grouped following the categories in the project subject.

### The World

- Seed-based terrain generated on demand through chunk streaming.
- Large-world exploration, including positions beyond 5,000,000 blocks on the XZ plane.
- Plains, cold plains, forests, cold forests, deserts, beaches, mountains, frozen mountains, oceans, and frozen oceans.
- Lakes, rivers, cave systems, coal, and diamond ores.
- Monsters that spawn and chase players, alongside passive animals.
- Moving 3D clouds that replenish around the player.
- Block destruction, item pickup, and block placement, with changes saved across chunk reloads.
- Nether portal.

### Graphic rendering

- Custom WebGPU renderer with camera-relative rendering for large coordinates.
- Chunk streaming, frustum culling, and merged terrain shadow meshes.
- Directional sunlight and dynamic shadows from terrain, entities, and clouds.
- Screen Space Ambient Occlusion (SSAO).
- Sky rendering, transparent water, and distance fog.

### Camera

- First-person movement and mouse-controlled camera.
- Walking, sprinting, jumping, and creative flight.
- Double-precision world positions for movement and collision handling at large coordinates.

### Sounds

- Biome music with smooth crossfades.
- Player and creature sound effects, including movement and combat.
- Distance-dependent sound attenuation.
- Stereo sound positional audio relative to the listener, alongside distance attenuation.

### Multiplayer and server

- Local world hosting and online multiplayer.
- Multiple connected players, synchronized player transforms, and entity states.
- Synchronized block placement and destruction, with persistent world saves.

### Interface

- Toggleable debug display with FPS, triangle, block, and chunk counts, position, and biome.
- Connected-player list, pause menu, and death screen with respawning.

### Other

- Gravity and collisions with solid blocks.
- Swimming and diving, with underwater visual effects.
- Walking, attacking, and death animations.
- Survival and creative modes.
- Crafting system.
- Bow and arrows.
- Cross-platform support.

## Controls

### Movement

| Key | Action |
|:---:|--------|
| `W` / `S` (QWERTY), `Z` / `S` (AZERTY) | Move forward / backward |
| `A` / `D` (QWERTY), `Q` / `D` (AZERTY) | Strafe left / right |
| Mouse | Look around |
| `Space` | Jump, swim up, or ascend in creative flight |
| `Left Ctrl` | Descend in creative flight |
| `Left Shift` | Sprint |

### Interaction

| Key | Action |
|:---:|--------|
| Left mouse button | Attack or destroy a block |
| Right mouse button | Place a block, interact, or use an item |
| Hold / release right mouse button | Charge / fire the bow |
| `1`–`9` / Mouse wheel | Select hotbar slot |
| `E` | Open / close inventory |

### Interface

| Key | Action |
|:---:|--------|
| `Esc` | Open pause menu or close inventory / chat |
| Left mouse button outside menus | Capture mouse and resume play |
| `T` | Open chat |
| `Enter` in chat | Submit message or command |
| `Tab` | Show connected players |
| Grave accent key | Toggle debug display |

Mouse sensitivity, sound-effects volume, and music volume can be adjusted in the pause menu.
Use `/gamemode creative` or `/gamemode survival` in chat to switch modes (host only).

## Commands

Press `T` to open chat, enter a command, then press `Enter`.

| Command | Action |
|---------|--------|
| `/give <item> [count]` | Add items to your inventory; count defaults to 1. |
| `/tp <x> <y> <z>` | Teleport yourself to integer coordinates in the current dimension. |
| `/dim <dimension>` | Switch dimension: `overworld` / `0` or `underworld` / `1`. |
| `/gamemode <mode>` | Set session mode: `survival` / `0` or `creative` / `1` (host only). |

## Technologies used

- **C++23** for the engine, gameplay, world generation, and server.
- **CMake** for builds and dependency downloads through FetchContent.
- **WebGPU / wgpu-native** and **WGSL** for GPU rendering and shaders.
- **SDL3** for windows and input.
- **SDL3_mixer** for audio playback and mixing.
- **Dear ImGui** for menus, settings, chat, and debug information.
- **ENet** for multiplayer networking.
- **GLM** for vector, matrix, and quaternion operations.
- **FreeType** for font rendering and **stb_image** for image loading.
- **zlib** for compression and **nlohmann/json** for JSON data.
- **MPSC_queue** for communication between threads.
- **Tracy** for profiling.

## Requirements

- A modern C++23 compiler and standard library:
  - **Linux:** Clang or GCC.
  - **macOS:** Clang.
  - **Windows:** MSVC only (Visual Studio C++ build tools).
- **CMake 3.20 or newer** and a compatible build tool.
- **Ninja** is recommended for fast builds.

## Installation

- Generate Release build files with Ninja and build:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel
```

## Run

Run from the repository root so the game can find its assets:

```bash
./build/ft_minecraft
```

With a multi-configuration generator such as Visual Studio, the executable is typically located at:

```powershell
.\build\Release\ft_minecraft.exe
```

Create or load a world from the main menu. Enable **Online** when hosting, or enter the host's IP address and select **Join**. World saves are stored in `appdata/saves/` relative to the working directory.


> [!NOTE]
Each player must use a unique name within the session.
