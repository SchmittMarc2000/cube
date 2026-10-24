# OpenCube — Project Plan (working codename)

A modern, open-source re-implementation of Cube World Alpha: original 64-bit C++20
engine that plays like the modded alpha, mounts your legally-owned install for
data, and makes modding a first-class feature instead of byte-patching a 32-bit exe.

---

## 1. Ground rules

- **100% original code.** No decompiling `Cube.exe`, no copying code or assets from it.
- **OpenMW model:** the engine reads data files (`resource1/2.dat`, models, defs) from a
  user-owned Cube World Alpha install at runtime. Nothing copyrighted ships in this repo.
- File formats and gameplay mechanics are re-implemented from community documentation
  and black-box testing against the reference install.

## 2. Locked decisions

| Decision | Choice | Rationale |
|---|---|---|
| Core language | **C++20** | Faithful to original, max performance, existing CW modders know it |
| Mod scripting | **Lua 5.4** (sandboxed, via sol2) | Safe hot-reload, huge modding audience, mods can't corrupt memory |
| Renderer | **DirectX 11 behind a thin RHI** | Long-term supported, zero practical downside at this scale; DX12 later = new backend, not rewrite |
| Assets | **Mount original install** | Zero art burden for parity; mods add content via PNG/glTF/OGG/JSON |
| Netcode | **Authoritative server from day 1** | SP = embedded local server; dedicated server binary is nearly free |
| Saves | **New async crash-safe format** (clean break) | Kills SQLite stutter permanently; no legacy import |
| Old mods | **Clean break** — new API only | Byte-patch DLLs die; bug fixes become engine behavior |
| v1 scope | Worldgen + 4 classes/pets + NPCs/quests/villages; minimal items | Deep crafting/adaptation + vehicles land right after v1 |

## 3. Architecture

```
opencube/                  # repo root (sibling of the game install)
  apps/
    opencube/              # client executable
    opencube-server/       # dedicated server executable (headless)
    tools/                 # format inspectors, asset viewers, dev harnesses
  engine/
    platform/              # SDL3 window/input/files/threads, 64-bit clean
    rhi/                   # thin rendering abstraction (D3D11 backend first)
    renderer/              # voxel chunk mesher, sprites, models, sky/lighting
    audio/                 # miniaudio backend (OGG etc.)
    ui/                    # RmlUi: HUD, menus, mod manager, debug tools
  game/                    # simulation (runs identically on client & server)
    worldgen/              # seeded deterministic terrain, biomes, landmarks
    entities/              # ECS: creatures, NPCs, pets, players
    combat/                # classes, specials, dodging, projectiles
    items/                 # minimal item/inventory/coins (crafting depth post-v1)
    quests/                # quest logic, villages, dungeons, shops
  formats/                 # readers for original install data (research-driven)
    resource_dat/          # resource1/2.dat archives
    models/                # original model/animation/sprite formats
    defs/                  # item/creature/etc. definitions inside archives
  net/                     # ENet-based client<->server protocol
  modapi/                  # Lua bindings, event bus, mod loader, native hook ABI
  tests/                   # unit + format golden tests against reference data
```

Key runtime properties:

- **Job system** for worldgen/meshing/saving off the render thread; frame budget target 16 ms.
- **Deterministic seeded simulation** (all RNG through seeded streams) — replays, testing, anti-cheat.
- **Event bus** at the core: gameplay emits events (`creature_damaged`, `item_crafted`, ...);
  achievements, quests, audio, UI, and mods all subscribe. This is also the mod API's backbone.
- **No SQLite anywhere.** New save format: journaled binary + atomic rename, async flush.

## 4. Modding design (the centerpiece)

### 4.1 Mod package layout
```
MyMod/
  mod.toml          # id, name, version, engine API version, deps, side (client/server/both)
  data/             # JSON defs: items, creatures, biomes, recipes, spawn tables, quests
  assets/           # PNG, glTF, OGG — open formats only
  lua/              # scripts
  native/           # optional C++ DLL (for heavy mods) using the stable hook ABI
```

### 4.2 Lua API sketch (sandboxed: no io/os, whitelisted bindings, per-mod env, instruction budget)
```lua
function on_creature_killed(evt)          -- every mod gets events via the bus
  if evt.creature:is_boss() then
    evt.killer:give_item("misc.spirit_cube", evt.creature.level)
  end
end

local biome = register_biome({            -- data-driven + programmatic hybrid
  name = "crimson_wastes", height = 40, temperature = 1.2,
})
```

### 4.3 Principles
- **Versioned API** (`api_version` in mod.toml) with capability negotiation — engine evolves without breaking mods silently.
- **Client/server mod split** mirroring the familiar `Mods/` vs `Server_Mods/` folders; server-authoritative mods flagged in metadata.
- **Hot reload** of Lua + data defs at runtime; mod manager UI with load order and dependency resolution.
- **Native hook ABI**: named hooks with stable signatures (the spiritual successor to the community's `CallbackManager.dll`) — no raw offsets, ever.
- Ship first-party example mods that demonstrate every API surface.

## 5. Built-in fixes & presets (absorbed from the community mod pack)

| Community mod | Becomes |
|---|---|
| TimeFixMod | Proper engine clock (no system-uptime overflow) |
| StutterFix | New async save system — no periodic hitches by design |
| NameOverflowFix | Real string handling; names are just strings |
| BadItemIDCrashFix | Safe item registry lookups with validation |
| MultiplayerPetFix | Pet XP/leveling is server-authoritative |
| AdaptionRebalance / EXPBuff | Optional "rebalance presets" in settings (vanilla curves remain selectable) |
| EfficentCraftingMod | Native crafting behavior when crafting ships (post-v1) |
| SpiritCubeGlow, TraderTravel, fast-travel saves | First-party example mods / QoL options |

## 6. Milestones & acceptance criteria

- **M0 — Foundation.** Repo scaffold, CMake presets, clang-format, CI (build + tests), dependency vendoring (SDL3, sol2/Lua, EnTT, glm, nlohmann/json, miniaudio, RmlUi, ENet, spdlog, Tracy), format-research docs against the reference install. *Done when CI is green with a hello-window.*
- **M1 — Core loop + data.** Platform layer, RHI/D3D11, input; `resource*.dat` reader renders one original model + terrain block palette; netcode skeleton (client connects to embedded server loopback). *Done when the window shows original-install assets and a heartbeat packet round-trips.*
- **M2 — World + saves.** Seeded infinite worldgen, biomes, chunk streaming on the job system, new async save/load. *Done when you can fly over endless terrain with no hitch >16 ms and reload the same world.*
- **M3 — Playable avatar.** Player controller, camera, class combat vs. test creatures, minimal items/inventory/coins. *Done when you can fight, loot, equip, and level.*
- **M4 — The world comes alive.** Creatures/NPCs, pets/taming, quests, villages, dungeons, day/night cycle. *Done when the v1 gameplay loop is complete solo.*
- **M5 — Multiplayer live.** Co-op over LAN/internet, dedicated `opencube-server`, server-authoritative pets/XP. *Done when two clients share a persistent world.*
- **M6 — v1 playable.** Polish, balance passes, options/UI completion. *Done when "it feels like modded alpha."* → then crafting/adaptation depth, vehicles (glider/boat/reins).
- **M7 — Mod API v1.** Public Lua API freeze, mod manager UI, hot reload, native hook ABI docs, first-party example mods (incl. the presets above). *Done when a third party can ship a content mod without touching engine code.*

## 7. Risks & mitigations

| Risk | Mitigation |
|---|---|
| Worldgen won't match original "feel" | Tune parameters by eye against the reference install; per-biome iteration milestones |
| Undocumented data formats | Research spike in M0/M1 with black-box diffing; unknowns fall back to replacement content |
| Scope creep (biggest risk) | Every milestone must end playable; post-v1 list is written down, not improvised |
| Solo-project stamina | Small timeboxed M0/M1; heavy automation (CI, golden tests, Tracy profiling) from day 1 |

## 8. Immediate next steps (start of M0)

1. Scaffold repo skeleton (as `C:\Users\Admin\Desktop\Git\OpenCube`, sibling to the install).
2. Toolchain: VS2022, C++20, CMake presets, clang-format, vcpkg/vendored deps, GitHub Actions.
3. Format research spike #1: `resource1.dat` structure, model format, `.plx` files (purpose unknown — investigate).
4. First commit: window + D3D11 clear + input + embedded server heartbeat.

---

*This plan is the roadmap for implementation. The engine requires a legally owned
Cube World Alpha install to run; no Picroma assets or code are included or derived from.*
