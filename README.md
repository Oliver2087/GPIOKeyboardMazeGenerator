# GPIOKeyboardMazeGenerator
MazeProject is a real-time 2D dungeon exploration game built using Qt (C++), QGraphicsView, and QGraphicsScene. The game features fully animated characters, procedural maze generation, key–door mechanics, and responsive movement with pixel-perfect collision detection.

Each level is generated using a custom MazeGenerator that constructs a multi-resolution maze grid (coarse + fine), places a randomized exit, distributes doors and corresponding keys, and ensures a valid path from the start to the exit. Mazes are generated asynchronously using QtConcurrent to prevent UI blocking, while a loading overlay with an animated GIF provides visual feedback.

The player character uses a multi-directional sprite system (front, back, left, right) with idle, walk, run, and attack animations loaded dynamically from disk. A custom PlayerItem class manages animation timing, frame updates, scaling, direction changes, and action transitions. To improve gameplay feel and prevent unfair collisions, the game implements a custom “feet-only” collision hitbox, separate from the visual sprite, providing natural movement around corners and walls.

Movement is driven by a controller class that supports continuous motion (≈60 FPS) using a timer-based update loop. Collisions are determined by scanning the scene using a feet-level hitbox to detect walls, doors, keys, and the exit region:

Walls: block movement

Locked doors: block movement until unlocked

Keys: unlock their corresponding door, remove themselves

Exit: triggers loading of a new maze level

Textures for walls, floors, doors, and keys are chosen randomly from curated tile sets to vary the appearance of each level. An exit texture (stone staircase) is scaled to match the maze’s 2×2 exit region for clarity and aesthetic cohesion.

The game supports multiple playable characters, each with their own sprite animations stored in external folders. The system is designed to easily add new characters or animation types without modifying core gameplay logic.

However a battle system is yet to be implemented.

GPIO or keyboard control can be changed by changing line 25 on gameview.h

#define CONTROL GPIO


