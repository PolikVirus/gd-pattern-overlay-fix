# GDR Pattern Reader (Geode)

Geode mod for Geometry Dash — read numbered click patterns from `.gdr` / `.gdr2` macros in-game.

**Web version:** sibling folder `gdr-click-pattern/` (unchanged).

See [CONCEPT.md](./CONCEPT.md) for architecture and roadmap.

## Install (start here)

**Full step-by-step guide:** [INSTALL.md](./INSTALL.md) — Geode setup, build, fix CL.exe crashes, copy `.geode` to mods folder.

Quick build (after `GEODE_SDK` is set):

```powershell
cd C:\Users\OasenHase\gdr-pattern-geode
geode build
```

Copy `build\oasenhase.gdr_pattern_reader.geode` → `%LOCALAPPDATA%\GeometryDash\geode\mods\`

## Usage

1. Enable the mod in Geode.
2. Main menu → **GDR Pattern** → load a `.gdr2` replay (`.gdr` xdBot requires a future MessagePack build).
3. Open a level and play your macro (or use the loaded inputs for analysis only).
4. Pause → **Pattern** to see mnemonics for the section defined in mod settings (% from / % to).

## Mod settings

| Setting | Description |
|---------|-------------|
| Level length (seconds) | Full level duration for % (0 = replay file; set e.g. 65 like web app) |
| Percent from | Start of analyzed section (in-game %) |
| Percent to | End of section |
| Player filter | P1, P2, or auto (dominant jump player) |

## Development

Core analysis mirrors:

- `gdr-click-pattern/src/analysis/analyzeSection.ts`
- `gdr-click-pattern/src/analysis/rhythmMemory.ts`

When changing algorithms, update **both** projects or add a shared test fixture later.
