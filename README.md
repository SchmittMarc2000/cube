# OpenCube

A modern, open-source re-implementation of the Cube World alpha engine: original
64-bit C++20 code, DirectX 11 rendering, an authoritative client/server split
from day one, and modding (Lua scripting + data-driven definitions) as a
first-class feature — no more byte-patching a 32-bit exe.

> **Legal:** OpenCube contains 100% original code and no Picroma assets. To run
> against original game data you must own Cube World and point OpenCube at your
> own install. Nothing here decompiles or distributes the original binaries.

Status: **M0 (foundation)** — window + D3D11 clear + input + embedded-server
heartbeat, unit tests, CI. See [PLAN.md](PLAN.md) for the full roadmap and
[docs/formats/research.md](docs/formats/research.md) for format findings.

## Building (Windows, VS2022+/VS2026)

```pwsh
# 1. Vendor SDL3 once (result is committed afterwards)
pwsh ./tools/fetch_sdl3.ps1

# 2. Configure + build
cmake --preset windows
cmake --build --preset windows-release

# 3. Run tests and smoke the client
ctest --preset windows-release
./build/windows/bin/Release/opencube.exe
./build/windows/bin/Release/opencube-server.exe
```

Client flags: `--frames N` (auto-exit, CI), `--warp` (software renderer, no GPU
needed), `--width/--height`, `--debug-layer` (D3D11 debug layer if installed).

## Repository layout

```
apps/        opencube (client), opencube-server (dedicated), tools (later)
engine/      base (log/clock), platform (SDL3 window), rhi (D3D11), renderer/audio/ui (M1+)
game/        simulation: worldgen, entities, combat, items, quests (M2+)
formats/     readers for original-install data formats (M1+)
net/         transport, wire protocol, embedded authoritative server
modapi/      Lua bindings, event bus, mod loader (M7)
tests/       unit tests
third_party/ vendored dependencies (SDL3)
```

## License

MIT — see [LICENSE](LICENSE).
