# INKBOUND: THE LAST REEL

A 2D action platformer / run-and-gun with boss battles, in a hand-drawn
early-20th-century cartoon world where living ink is drying up. Written in
**C++20**, built with **CMake + SDL2** (the only dependency), 100% original IP.

> **Current status: vertical slice complete** — one world (Meadows) with two
> levels, three enemy types, a mini-boss (The Patchling), a full multi-phase
> boss (Barnaby Patchface), parry, dash, super, checkpoints, save system,
> ranking, endings and the credit roll. The architecture is the final
> architecture: new enemies, bosses, weapons and levels are **data** (JSON /
> `.lvl`), not code.

![gameplay](docs/screenshot_gameplay.png)

---

## Requirements

- C++20 compiler (MSVC 2022/2026, GCC 11+, Clang 14+)
- CMake ≥ 3.16
- SDL2 ≥ 2.26 (development)

## Building

### Windows (MSVC, the primary target)

```bat
:: SDL2 via vcpkg (or set INK_SDL2_PREFIX to any SDL2 install)
C:\vcpkg\vcpkg install sdl2:x64-windows
cmake -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Release -DINK_SDL2_PREFIX=C:/vcpkg/installed/x64-windows
cmake --build build --config Release
```

Run: `build\Release\inkbound.exe Assets`

### Windows (MinGW-w64)

```bat
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DINK_SDL2_PREFIX=C:/msys64/mingw64
cmake --build build
```

### Linux

```sh
sudo apt install libsdl2-dev cmake ninja-build
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Development -DINK_WITH_TESTS=ON
ninja -C build
./build/inkbound Assets
```

### Build flavours

| Flavour       | Optimisation | Asserts | Dev tools (F1/F2/F12, debug overlay) |
|---------------|--------------|---------|---------------------------------------|
| `Debug`       | `-O0 -g3`    | yes     | yes                                   |
| `Development` | `-O2 -g`     | yes     | yes (default)                         |
| `Release`     | `-O2`        | no      | no                                    |
| `Shipping`    | `-O3`        | no      | no, shipping flags                    |

`INK_WITH_TESTS=ON` (default) builds `inkbound-tests`; run it with
`ctest --test-dir build`.

## Controls

| Action   | Default | Notes |
|----------|---------|-------|
| Move     | A/D or ←/→ | run |
| Jump     | W or Space | variable height, coyote time, jump buffering |
| Crouch / drop through | S | |
| Attack   | J | hold to charge (Ink Blaster) |
| Dash     | K | i-frames, air dash |
| Parry    | L | 0.14s window; parries projectiles and melee |
| Special  | I | Inkstorm when the super meter is full |
| Pause    | Esc | |
| F1 / F2 / F12 | — | hitboxes / god mode / screenshot (dev builds) |

All bindings are remappable in the Options menu and persisted to the save.

## Game systems in the slice

- **Simulation/render decoupling**: fixed 60 Hz simulation, render at display
  rate; timescale effects (hit-stop, slow-mo) are part of the sim clock.
- **Player**: state machine (idle/run/jump/fall/crouch/dash/air-dash/attack/
  parry/hurt/super/dead), coyote time, jump buffering, parry with hit-stop
  feedback, dash i-frames, super meter, revive charm support.
- **Enemies**: data-driven AI (slome, inkbat, quillgunner, wisp) with
  telegraphed attacks that are parryable.
- **Bosses**: multi-phase with telegraph zones, phase music layers, rank
  (S+…D) computed from time/damage/parries/no-hit, records persisted.
- **Levels**: ASCII tile maps in `.lvl` (`.#=^?Di` + entity chars
  `P C B i f m s`), moving platforms, checkpoints, hazards, boss zone,
  exit zone, fall-out-of-world safety respawn.
- **Save system**: 3 slots, versioned, CRC32 corruption protection,
  atomic write (temp + rename), settings sidecar.
- **Audio**: procedural music/SFX (no external files), per-track layers for
  boss phases, volume options.
- **Presentation**: hand-drawn bitmap art generated in code into a sprite
  atlas, parallax backgrounds, film-grain FX, cinematic letterbox cards,
  bitmap font (with accented glyphs for localized names), credit roll.
- **Meta**: worlds/levels/achievements/quests/charms/collectibles all JSON
  data-driven; 3 endings; ending credit roll from `Assets/Data/credits.json`.

## Headless QA

The game can run fully headless (dummy video/audio drivers) with a
deterministic test bot — used for continuous QA:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  INK_AUTOTEST=1 INK_AUTOQUIT=6000 \
  INK_SHOTS="300:/tmp/f1.png,600:/tmp/f2.png" \
  ./build/inkbound Assets
```

- `INK_AUTOTEST=1` — start a new game and let the bot play (walk/jump/
  attack/dash/parry on fixed timers, auto-advance all screens).
- `INK_AUTOTEST=2` — continue the latest save (useful for long runs).
- `INK_AUTOQUIT=N` — quit after N simulation frames.
- `INK_SHOTS="frame:path,..."` — capture raw frames (A,B,G,R byte order) at
  exact simulation frames.

## Repository layout

```
Source/
  Core/        math, rng, json, crc, events, log          (no SDL)
  Physics/     tile AABB collision (MoveEntity, LOS)      (no SDL)
  AI/          enemy steering helpers
  Animation/   anim references & timing
  VFX/         particle system
  Engine/      application loop, app config
  Rendering/   camera, 2D renderer, sprite atlas bank
  Input/       logical actions <-> physical key binds
  Audio/       audio engine, procedural music & SFX
  UI/          bitmap font, HUD, menus, screens (rank, ending, credits)
  Art/         in-code pixel art (player, enemies, bosses, tiles, bg)
  Gameplay/    sim context, game events, level model
  Player/      player controller & stats
  Enemies/     enemy base + types + data book
  Bosses/      boss base + types + data book
  Weapons/     weapon/projectile definitions
  Charms/      charm definitions & stat folding
  Items/       pickup item definitions
  Levels/      .lvl parser
  World/       world/level registry
  Save/        save system (versioned, CRC-protected)
  Cinematics/  letterbox text cards
  Progression/ meta progression & counters
  Quests/      quest data & tracking
  Dialogue/    NPC dialogue
  Game/        the Game orchestrator (frame driving, event->feedback)
  Tools/       debug overlay
  Editor/      level editor tool (separate executable)
  Tests/       unit tests (json, parser, physics, rng, save, data books)
Assets/
  Data/        *.json (weapons, enemies, bosses, charms, worlds, credits…)
  Levels/      *.lvl
```

## Roadmap to the full version

The slice validated the core; the prescribed expansion order:

1. World 2 (Carnival) — 4 levels, 4 enemy types, 1 mini-boss, 2 bosses
2. Worlds 3–6 (Bay, City, Kingdom, Frame) — full 25+ level / 18 boss roster
3. 12+ weapons & 25+ charms with meaningful build choices
4. Boss rush / nightmare / speedrun / no-hit modes
5. 50+ achievements, 20+ NPCs, full quest/dialogue web, 3 endings
6. Localization (string tables), accessibility pass, performance pass
   (pools/batching, zero-allocation combat)

Everything needed for the expansion is a data file plus, for new species,
an `Enemy::Create`/`Boss::Create` factory entry and an art generator — no
core rewrites.

## Credits

In-game end credits come from `Assets/Data/credits.json` (the team is
listed there). All names, art, music and design in this project are
original; the only external dependency is SDL2 (zlib license).
