# Format research

Findings from black-box inspection of the reference install
(`C:\Users\Admin\Desktop\Git\Cube`, Cube World alpha + community mod pack).
All findings derived from file headers only — no decompilation. Update this
file as each milestone pins down more structure.

## Reference install inventory

| File | Size | Identified as |
|---|---|---|
| `Cube.exe`, `Server.exe` | 32-bit | Original game binaries (never touched by this project) |
| `resource1.dat`, `resource2.dat` | ~19 KB, ~16 KB | **TrueType/OpenType fonts** (sfnt `0x00010000`, tables: OS/2, cmap, gasp, glyf, head...) — *not* archives |
| `data1.db` ... `data4.db` | ? | **SQLite 3** databases (header magic `SQLite format 3\0`) |
| `db.dat` | 64 B | ASCII placeholder: `PLACEHOLDER00000...` — a stub that never shipped |
| `*.plx` (`start`, `gui`, `cursor`, `help`, `quest-tag`) | small | Custom container, see below |
| `Save/*.db` | varies | SQLite saves: `worlds.db`, `world_*.db`, `map_*.db`, `characters.db` |
| `options.cfg` | text | Key/value settings (`resolutionX 2560` etc.) |
| `server.cfg` | 5 B | Single number: the server port (`26879`) |

## The `.plx` container (open question, M1 priority)

All `.plx` files share a header shape:

```
00000000  00 00 00 00 16 00 00 00 01 00 00 00 0E 00 00 00
          [u32 zero ] [u32 0x16 ] [u32 1   ] [u32 len  ] "PlasmaGraphics\0"...
00000020  ... 04 00 00 00 01 00 00 00 00 00 00 00 0C 00 00 00
          [ ... ]     [u32 4    ] [u32 1    ] [u32 0    ] [u32 12   ] [u32 2    ] [u32 4    ] "Seal\0"
00000040  ... 12 00 00 00 0E 00 00 00 78 87 81 6D BD 3D 2F C7 6E 99 AC 39 28 D1 00 00 00 00
          [u32 0x12  ] [u32 14   ] [14 raw bytes: 78 87 81 6D BD 3D 2F C7 6E 99 AC 39 28 D1]
```

Hypotheses:

- "PlasmaGraphics" is the engine's own name (Wollay's tech was branded *Picroma
  Plasma*), so `.plx` = Plasma-something (script? layout? compiled UI?). `gui.plx`,
  `cursor.plx`, `help.plx`, `start.plx`, `quest-tag.plx` all being small UI-adjacent
  names supports a UI-script/layout role.
- Structure looks TLV: repeated `[u32 tag][u32 len][data]` records. `Seal` (len 14)
  may be an integrity hash/signature over what follows — or over the whole body.
- `start.plx` and `gui.plx` have *identical* first 0x40 bytes except the total-length
  fields — same container, same metadata; likely same kind of payload.

Next steps (M1 research spike):

1. Full hexdump + entropy analysis of every `.plx` body (compressed? encrypted? high
   entropy ⇒ encryption, low/structured ⇒ compression or bytecode).
2. Diff the `Seal` records across all five files; check whether flipping a body byte
   changes the seal (would confirm hash-over-body).
3. Check community documentation (cw modding scene wikis/repos) before burning hours
   on black-box work — the alpha modders reverse-engineered much of this already.

## Where is the actual game data?

Open question. Models/textures/definitions are **not** loose files in the install
root. Candidates:

1. Inside `data1-4.db` (SQLite) — cheap to check: enumerate tables (read-only, on a
   copy of the files).
2. Compiled into `Cube.exe` as resources — community knowledge says large parts of
   alpha data live in the executable; we do **not** read the executable. If this
   turns out to be the case, the engine ships with replacement content instead and
   only borrows the *behavior* of originals.
3. Procedurally generated at runtime (fonts being the exception) — partially true for
   the landscape by design (seeded worldgen), but creature/item definitions must be
   stored somewhere.

Priority for M1: dump `data*.db` table schemas (via a scratch copy, never the
originals in place).

## Rules of engagement

- Never write to the reference install. Copy files out, then experiment.
- Never load `Cube.exe`/`Server.exe` into a disassembler for this project.
- Document every finding here with offsets, endianness, and confidence level.
