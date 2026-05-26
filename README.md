<div align="center">

<picture>
  <img alt="OrcaSlicer logo" src="resources/images/OrcaSlicer.png" width="15%" height="15%">
</picture>

# OrcaSlicer — FilamentVault

**Filament inventory management built into OrcaSlicer**

</div>

## What is FilamentVault?

FilamentVault is a **filament stock tracking system** integrated directly into OrcaSlicer. It lets you register all your physical filament spools and automatically deducts the used weight every time you print.

### Features

- **Spool inventory** — Register spools with brand, material, color, weight, and custom temperature overrides
- **Automatic deduction** — When you slice and print, the filament weight is automatically subtracted from the selected spool
- **Consumption history** — Every deduction is logged with timestamps
- **Visual cards** — Color-coded cards with material badges and remaining weight progress bars
- **Search & filters** — Filter by material type or search by name/brand
- **Archiving** — Archive empty spools without deleting them
- **Custom presets** — Each spool generates a printer preset with the correct temperatures and density
- **Cross-platform** — Works on Linux, Windows, and macOS

### How it works

1. Open the **Filament** tab in OrcaSlicer
2. Add your spools (brand, material, color, total weight)
3. Select a spool and click **"Print with"** to set it as active
4. Slice and print as usual — the used weight is deducted automatically

The filament weight (`total_weight` in grams) is computed from the extruded volume and filament density during slicing, so the deduction is accurate.

## Data storage

FilamentVault uses a **local SQLite database** stored in your OrcaSlicer config directory:

| Platform | Path |
|----------|------|
| Linux | `~/.config/OrcaSlicer/filament_vault.db` |
| Windows | `%APPDATA%/OrcaSlicer/filament_vault.db` |
| macOS | `~/Library/Application Support/OrcaSlicer/filament_vault.db` |

Each user has their own database — nothing is shared or uploaded.

## Deduction triggers

Weight is automatically deducted when:

- ✅ **Export G-code** (File → Export → G-code)
- ✅ **BBL Network Print** (Print Plate / Print All)
- ✅ **Send to SD card**
- ✅ **Legacy send** (Print Host)

## Building from source

### Linux

```bash
mkdir build && cd build
cmake ..
cmake --build . --config RelWithDebInfo --target all -- -j$(nproc)
```

### Windows (Visual Studio 2022)

```
build_release_vs2022.bat
```

### macOS

```bash
mkdir build && cd build
cmake ..
cmake --build . --config RelWithDebInfo --target all --
```

## Requirements

- CMake 3.13+
- C++17 compiler
- wxWidgets
- Boost
- SQLiteCpp (included via `cmake/modules/FindSQLiteCpp.cmake`)

## License

This project is based on [OrcaSlicer](https://github.com/SoftFever/OrcaSlicer) and retains its license.
