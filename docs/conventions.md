# Code conventions

## C++

- C++20. MSVC-first (`/W4 /permissive- /utf-8`), but keep code GCC/Clang-clean
  for the future cross-platform build.
- Namespaces: `oc::<subsystem>` (`oc::log`, `oc::platform`, `oc::rhi`, `oc::net`).
- Types `PascalCase`; functions/variables `snake_case`; members `m_`-prefixed;
  constants `k`-prefixed (`kProtocolVersion`); macros `OC_`-prefixed.
- Headers: `snake_case.hpp` matching source names. Includes written as
  `"engine/base/oc_log.hpp"`, `"net/net_protocol.hpp"` (repo root is the include root).
- Include order: (1) matching header, (2) project headers, (3) third-party,
  (4) std, separated by blank lines.
- Ownership: smart pointers; no raw `new` outside constructors of owning wrappers.
- Errors (M0 style): return `bool` + log. Migrate to `std::expected` where it
  clarifies APIs. Never silently swallow failures.
- Threading: all shared state behind documented invariants; prefer message
  passing (see `oc::net::Loopback` for the pattern).
- Determinism: gameplay RNG goes through seeded streams (M2+, `game/`), never
  `rand()`. `engine/base/oc_clock.hpp` for all timing.

## CMake

- One `CMakeLists.txt` per library/app. Static libs named `oc<layer>`
  (`ocbase`, `ocplatform`, `ocrhi`, `ocnet`).
- Dependencies: vendored under `third_party/`, committed (small) so clones and
  CI build offline after the initial fetch.
- Use presets (`cmake --preset windows`), never raw `-G` incantations.

## Git

- Conventional-ish subjects: `rhi: fix swapchain resize on minimize`,
  `net: add heartbeat protocol`. Imperative mood, lowercase after prefix.
- Never commit: build outputs, user settings, reference-install data (anything
  from the original game's files). `third_party/` is the exception by policy.

## Project principles

1. **No raw offsets ever.** Anything a mod could want is a named, documented API.
2. **The reference install is read-only.** Copy files out before experimenting.
3. **Every milestone ends runnable.** Broken intermediate states live on branches.
4. Gameplay data belongs in data files (`data/`) or Lua, not hard-coded.
