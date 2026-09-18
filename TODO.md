
## V.1: The World

- [x] The world must be generated on demand.
- [x] You should be able to navigate through at least 5,000,000 cubes on the XZ plane.
- [ ] The terrain should not be uniform; you must implement different biomes like mountains, canyons, islands, etc.
    - [ ] Add island/canyon biomes
- [x] A minimum of 5 unique biomes is required
- [x] Each biome should have unique geography, elevation, vegetation, and distinct characteristics that make them feel truly unique.
    - [x] Spruce trees in cold forests
    - [ ] Add different tint coords for different biomes
- [x] Biomes should transition smoothly and naturally without abrupt changes, as illustrated below
- [ ] There should be small plants, flowers, and mushrooms scattered throughout the world, as well as procedurally generated trees.
    - [ ] Mushroom ?
- [ ] There must be lakes and rivers meandering across the world, as well as natural cave entrances visible from the surface.
    - [x] Lakes
    - [ ] River: currently not perfect and goes up montains.
    - [x] Caves
- [x] These caves should feature realistic formations (wormhole style) and contain clusters of rare ores like gold and diamonds, not just simple noise-based distribution
    - [x] Wormhole caves
    - [x] Ore generation
- [x] Monsters (like creepers or zombies) should spawn and chase you when you get close.
- [x] 3D clouds should float across the world. They can either be represented as blocks (purely visual with no interaction) or as shaders.
- [ ] You should be able to pick up blocks after destroying them (just like in Minecraft) and place them wherever you want.
- [x] Destroyed or placed blocks must be persistent.

## V.2: Graphic rendering

- [x] Minimum render distance is 260 ( 16 chunks )
- [x] You may use a sky shader instead of a skybox if desired.
- [x] Directional lighting
- [ ] Shadows
- [ ] Screen Space Ambient Occlusion (SSAO)
- [x] Transparent water surfaces
- [x] Far distance fog for better immersion

## V.3: Camera

- [x] Jump and sprint actions
- [x] 360-degree mouse control on the Y-axis, with the ability to look up and down
- [x] Walking speed of approximately 1 cube per second, and 2 cubes per second when sprinting
- [x] A toggleable fly-mode, with running speed multiplied by 20 when flying

## V.4: Sounds

- [ ] Each biome should have its own unique ambient music, with smooth transitions between them.
- [x] Both players and monsters must have sounds for actions like walking, attacking, and swimming.
- [x] The sound volume should dynamically adjust based on distance from the source.

## V.5: Multiplayer

- [x] Your server should allow at least four players to join simultaneously.
- [ ] Players should be visible in the world, performing any actions such as walking, attacking, destroying blocks, and even getting killed by monsters
    - [ ] Get killed by monsters
- [x] All modifications to the world (block placement, block destruction) must be synchronized across all players and persistent even after reloading.
    - [x] Entity states (like monsters) should also be synchronized

## V.6: Interface

- [x] FPS, triangles, cube, and chunk counts must be displayed on-screen with a key toggle.
- [x] A list of all connected players should also be available with a key toggle.

## V.7: Other

- [x] A simple gravity system that handles block collisions (excluding water).
- [x] The ability to swim and dive, with optional slowed movement underwater.
- [x] Visual rendering adaptations for underwater exploration (color filters, reduced visibility).
- [ ] Basic animations for walking and attacking, Minecraft-like in simplicity.
