# Installing GDR Pattern Reader (Geode mod)

Step-by-step guide for **Windows** and Geometry Dash **2.2081** with Geode.

The mod mirrors the **web app** (`gdr-click-pattern`): same click analysis, **240 TPS** timing, level % with optional **level length (seconds)** in settings.

---

## 1. Prerequisites (one-time)

### A. Geometry Dash 2.2081

- Steam or other copy on **2.2081** (Geode targets this version).
- In Geode settings, confirm the game version matches.

### B. Geode Loader

1. Download the installer from [geode-sdk.org](https://geode-sdk.org/).
2. Run it and point it at your Geometry Dash folder.
3. Launch GD once through Geode so the `geode` folder and mod directory exist.

Typical mod folder:

`%LOCALAPPDATA%\GeometryDash\geode\mods\`

### C. Node IDs dependency

This mod requires **Node IDs**:

1. Open GD → Geode → **Download** (or website).
2. Install **Node IDs** (`geode.node-ids`).
3. Enable it in the mod list.

### D. Build tools (only if you compile yourself)

| Tool | Purpose |
|------|---------|
| [Geode CLI](https://docs.geode-sdk.org/getting-started/sdk/) | `geode` on PATH |
| Geode SDK clone | `GEODE_SDK` env variable |
| Visual Studio 2022 | C++ workload (“Desktop development with C++”) |

**Recommended:** Visual Studio **2022** (17.x), not VS 18 preview, if the compiler crashes during build.

---

## 2. Set GEODE_SDK (one-time)

1. Clone the SDK (if you have not):

   ```powershell
   git clone https://github.com/geode-sdk/sdk.git C:\Users\OasenHase\Documents\Geode
   ```

2. **System environment variable** (Windows):

   - Win + R → `sysdm.cpl` → **Advanced** → **Environment Variables**
   - Under **User variables** → **New**
   - Name: `GEODE_SDK`
   - Value: `C:\Users\OasenHase\Documents\Geode` (your SDK path)
   - OK, then **restart PowerShell / Cursor**

3. Verify:

   ```powershell
   echo $env:GEODE_SDK
   geode --version
   ```

---

## 3. Build the mod

Open **PowerShell** (not CMD):

```powershell
cd C:\Users\OasenHase\gdr-pattern-geode
```

### Recommended: use the build script (fixes VS 18 / CL.exe crash)

If you have **both** Visual Studio 2022 and **Visual Studio 2026 (18)** installed, a plain `geode build` often picks **VS 18** and fails with:

`error MSB6006: "CL.exe" wurde mit dem Code -1073741819 beendet`

That is a compiler/PDB crash, not a bug in this mod. **Do not use bare `geode build` after a failed attempt** — use:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Clean
```

The script deletes `build\`, pins **Visual Studio 17 2022**, and runs `geode build` inside the VS 2022 toolchain.

First-time or after errors: always pass **`-Clean`**.

### Manual clean (if you prefer)

```powershell
taskkill /IM mspdbsrv.exe /F 2>$null
Remove-Item -Recurse -Force .\build -ErrorAction SilentlyContinue
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

Requirements:

1. **Visual Studio 2022** Build Tools with **MSVC v143** and **Windows 10/11 SDK** (you already have this if the VS 2022 Developer Prompt works).
2. `GEODE_SDK` set (see step 2).
3. Optionally exclude `gdr-pattern-geode\build` from real-time antivirus during the first compile (GeodeBindings takes several minutes).

### Successful build

You should see something like:

`| Success | Built oasenhase.gdr_pattern_reader`

The packaged file is usually:

`build\oasenhase.gdr_pattern_reader.geode`

(or under `build\package\` — check the last lines of `geode build` output).

---

## 4. Install the mod into Geometry Dash

You only need this when you **first install** or after you **change the code** and rebuild.

### Easiest: let Geode CLI install for you (recommended)

From the project folder in PowerShell:

```powershell
cd C:\Users\OasenHase\gdr-pattern-geode
geode build --config Release
```

Watch the last lines of the output:

- **`| Done | Installed oasenhase.gdr_pattern_reader.geode`** — the CLI copied the package into your Geode mods folder. You do **not** need to copy files by hand.
- If you do **not** see “Installed”, use manual copy below.

Then **start Geometry Dash** (through your usual shortcut/Steam/Geode). You do **not** run any extra command every time you open the game.

### Manual copy (if you built without auto-install)

1. Find the packaged file **`oasenhase.gdr_pattern_reader.geode`**. Search under `build\` (Geode may also print the exact path when packaging).
2. Copy that **single .geode file** into Geode’s mods directory:

   `C:\Users\OasenHase\AppData\Local\GeometryDash\geode\mods`

   Quick open: Win + R → paste `%LOCALAPPDATA%\GeometryDash\geode\mods` → Enter.

3. If an older folder exists (e.g. `oasenhase.gdr_pattern_reader`), you can leave it; installing a new `.geode` from the Geode UI or CLI normally updates the mod. If something acts stale, remove that mod’s folder and **install the fresh `.geode` again**, then restart GD.

4. Launch GD.

### Updating after code changes

1. Rebuild: `geode build --config Release` (or `.\build.ps1` from this repo).
2. If the CLI reported **Installed**, restart GD (fully quit, then open again). If not, copy the new `.geode` over the old one in `mods` and restart.

### Development symlink (optional)

```powershell
geode project link
```

Then `geode build` can install into the linked location; restart GD after builds.

### If `geode` is not recognized

Install [Geode CLI](https://docs.geode-sdk.org/getting-started/sdk/) and reopen your terminal, or run `geode` from the folder where it was installed (e.g. via WinGet).

---

## 5. Enable and configure in-game

1. Geode → **Mods** → enable **GDR Pattern Reader**.
2. Enable **Node IDs** if prompted.
3. Open mod **Settings**:

   | Setting | What to set |
   |---------|-------------|
   | **Level length (seconds)** | Full level time (e.g. `65`). Use `0` to use the replay file only. |
   | **Percent from / to** | Section of the level to analyze (same idea as the web timeline). |
   | **Player filter** | `auto`, `p1`, or `p2` |

4. **Restart GD** after changing settings if the mod does not pick them up.

---

## 6. How to use (matches web workflow)

1. **Main menu** → bottom bar → **GDR** → pick a **`.gdr2`** replay.
   - MegaHack / bot exports are usually `.gdr2`.
   - xdBot **`.gdr`** needs export as `.gdr2` or use the web app.
2. Open your level and play (or just pause for analysis).
3. **Pause** → **Pattern** → toggles an **in-game timeline overlay**:
   - Full-level **click bars** and **holds**, **% ticks**, and a **white playhead** tied to your current **level %** (stays in sync with practice respawn / start position).
   - Drag the **top strip** of the overlay to move it; **Pattern** again to hide it.

---

## 7. Troubleshooting

| Problem | Fix |
|---------|-----|
| Mod not in list | `.geode` not in `geode\mods\`; wrong GD version; rebuild. |
| “Node IDs” / hook errors | Install and enable **geode.node-ids**. |
| No **GDR** button | Mod disabled; conflict with another menu mod; check Geode log. |
| **Pattern** empty | Load a replay first; widen % range; check player filter. |
| Wrong **%** | Set **Level length (seconds)** like on the web (e.g. 65). |
| Build **MSB6006** / CL crash | Clean `build\`, use VS 2022, `/FS` flags (already in CMakeLists), kill `mspdbsrv`. |
| `.gdr` not supported | Export **.gdr2** from your bot or use the web app. |

**Geode log:** Geode → Settings → Open log folder — search for `gdr_pattern` or errors.

---

## 8. Web vs mod (same logic)

| Feature | Web | Geode mod |
|---------|-----|-----------|
| Click patterns / mnemonics | Yes | Yes |
| 240 TPS when file says 60/360 | Yes | Yes |
| Level length override | Yes | Mod setting |
| Play clicks + song | Yes | No (view patterns only) |
| Timeline / practice | Yes | In-game overlay (pause → Pattern) |

Algorithm source: `src/analysis/` in this repo and `gdr-click-pattern/src/analysis/` in the web repo.
