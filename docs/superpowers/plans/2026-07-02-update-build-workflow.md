# Update build.yml — All Combos + Artifact Upload

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Expand CI to build all 4 combos (Win32/x64 × Debug/Release) + export .lib/.exe/.h as downloadable artifacts

**Architecture:** Add `config` to GitHub Actions matrix → gate test steps to Debug only → parameterize build paths → copy built files to `artifacts/` → upload with `actions/upload-artifact@v4`

**Tech Stack:** GitHub Actions, CMake (VS multi-config generator), bash, actions/upload-artifact@v4

## Global Constraints

- Keep `actions/checkout@v1` as-is (not in scope)
- All existing test functionality preserved for Debug builds
- `bash` shell for steps needing fail-fast (existing pattern)

---

### Task 1: Update `.github/workflows/build.yml`

**Files:**
- Modify: `.github/workflows/build.yml`

**Interfaces:**
- Consumes: existing build structure
- Produces: 4 artifact bundles per push/PR (`luau-build-Win32-Debug`, `luau-build-Win32-Release`, `luau-build-x64-Debug`, `luau-build-x64-Release`)

- [x] **Step 1: Add `config` to matrix**

Add `config: [Debug, Release]` to the strategy matrix alongside `arch`.

- [x] **Step 2: Gate test steps to Debug only**

Add `if: matrix.config == 'Debug'` to:
- `cmake build` (test targets)
- `run tests`
- `run extra conformance tests`

- [x] **Step 3: Parameterize CLI build paths**

Change `--config Debug` → `--config ${{matrix.config}}` and `Debug/` → `${{matrix.config}}/` in the CLI step.

- [x] **Step 4: Add artifact copy step**

Use bash to create `artifacts/lib/<arch>/<config>/` and `artifacts/bin/<arch>/<config>/`, then copy `.lib` and `.exe` files from the build output directory.

- [x] **Step 5: Add header export step**

Copy public headers from module `include/` dirs to `artifacts/include/`.

- [x] **Step 6: Add upload step**

Use `actions/upload-artifact@v4` with name `luau-build-${{matrix.arch}}-${{matrix.config}}` and path `artifacts/`.

#### Final file content

```yaml
name: build

on:
  push:
    branches:
      - "master"
      - "hi-de"
    paths-ignore:
      - "docs/**"
      - "papers/**"
      - "rfcs/**"
      - "*.md"
  pull_request:
    paths-ignore:
      - "docs/**"
      - "papers/**"
      - "rfcs/**"
      - "*.md"

jobs:
  windows:
    runs-on: windows-latest
    strategy:
      matrix:
        arch: [Win32, x64]
        config: [Debug, Release]
    steps:
      - uses: actions/checkout@v1
      - name: cmake configure
        run: cmake . -A ${{matrix.arch}} -DLUAU_WERROR=ON -DLUAU_NATIVE=ON
      - name: cmake build tests
        if: matrix.config == 'Debug'
        run: cmake --build . --target Luau.UnitTest Luau.Conformance --config Debug
      - name: run tests
        if: matrix.config == 'Debug'
        shell: bash
        run: |
          Debug/Luau.UnitTest.exe
          Debug/Luau.Conformance.exe
          Debug/Luau.UnitTest.exe --fflags=true
          Debug/Luau.Conformance.exe --fflags=true
      - name: run extra conformance tests
        if: matrix.config == 'Debug'
        shell: bash
        run: |
          Debug/Luau.Conformance.exe -O2
          Debug/Luau.Conformance.exe -O2 --fflags=true
          Debug/Luau.Conformance.exe --codegen
          Debug/Luau.Conformance.exe --codegen --fflags=true
          Debug/Luau.Conformance.exe --codegen -O2
          Debug/Luau.Conformance.exe --codegen -O2 --fflags=true
      - name: cmake cli
        shell: bash
        run: |
          cmake --build . --target Luau.Repl.CLI Luau.Analyze.CLI Luau.Compile.CLI --config ${{matrix.config}}
          ${{matrix.config}}/luau tests/conformance/assert.luau
          ${{matrix.config}}/luau-analyze tests/conformance/assert.luau
          ${{matrix.config}}/luau-compile tests/conformance/assert.luau
      - name: copy artifacts
        shell: bash
        run: |
          arch="${{matrix.arch}}"
          config="${{matrix.config}}"
          mkdir -p "artifacts/lib/$arch/$config" "artifacts/bin/$arch/$config"
          cp "$config"/*.lib "artifacts/lib/$arch/$config/" 2>/dev/null || true
          cp "$config"/*.exe "artifacts/bin/$arch/$config/" 2>/dev/null || true
      - name: export headers
        shell: bash
        run: |
          mkdir -p artifacts/include
          for module in VM Common Compiler Ast Bytecode Analysis CodeGen Config Require; do
            if [ -d "$module/include" ]; then
              cp -r "$module/include/"* artifacts/include/ 2>/dev/null || true
            fi
          done
      - name: upload artifacts
        uses: actions/upload-artifact@v4
        with:
          name: luau-build-${{matrix.arch}}-${{matrix.config}}
          path: artifacts/
```

## Self-Review

- [x] **Spec coverage:** 4 combos ✓, artifacts upload ✓, tests preserved on Debug ✓
- [x] **Placeholder scan:** No TBD/TODO/placeholder
- [x] **Type consistency:** `${{matrix.config}}` used consistently for paths and build config; condition `matrix.config == 'Debug'` matches gating logic
