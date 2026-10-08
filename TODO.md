# TODO

Living task list. Historical context for the mh_stuff work lives in `migration.md`.

## HTTP transport migration

cpprestsdk was removed because it is archived, deindexed from upstream vcpkg, and
incompatible with newer MSVC headers. The initial cpp-httplib replacement then hit
its Windows 10 minimum; the application still targets Windows 8.1.

**Current transport: libcurl's easy interface.** `HTTPClient.cpp` keeps its twelve
workers, per-host dispatch throttling, coroutine completion, retry policy, and
request counters. Each worker owns its per-origin curl handles and reuses their
connections. Handles are never used concurrently; the earlier objection about a
shared per-host cache no longer applies to this worker-owned cache.

The callback, checked curl options, and handle/global lifecycle add some adapter
code, but preserve the existing public `IHTTPClient` interface and call sites.
The vcpkg `curl[ssl,brotli]` dependency disables default features: HTTP/HTTPS only,
Schannel on desktop Windows, OpenSSL on Linux, and gzip/Brotli decoding. Requests
require TLS 1.2 or later and verify certificates and hostnames.

- [x] Remove cpprestsdk from production code and dependencies.
- [x] Replace the production httplib client with libcurl; retain the worker/retry flow.
- [x] Keep the Windows 8.1 API target rather than raise it for the HTTP library.
- [x] Make cpp-httplib an optional, TLS-free loopback server fixture through the
      `http-tests` vcpkg feature, selected by `TF2BD_ENABLE_HTTP_TESTS` before `project()`.
      Only that fixture translation unit targets Windows 10 on Windows; production
      HTTP sources keep the application's API target. Windows tests check Schannel.
- [x] Build Linux Release, pass the six loopback transport test cases, and verify
      HTTPS against the public GitHub release endpoint with certificate checks enabled.
- [ ] Validate the exact Windows release build and its live API requests on Windows
      8.1 and, as a best effort, Windows 7 SP1. The API target alone cannot guarantee
      compatibility of the compiler runtime and all bundled dependencies.
- [ ] Audit MSVC/runtime compatibility before moving Windows CI to `windows-latest`.
      Keep `windows-2022` for now to avoid accidentally raising the release floor.

### Follow-on: one ImGui frontend for native and browser builds

This is a larger, separate refactor than replacing the outbound HTTP client above. The
native application would serve the browser build and expose local state/actions over HTTP;
the same `MainWindow.cpp` (and its supporting UI code) should compile in both targets.

- [ ] Define a browser-safe UI state model and action interface. `MainWindow` currently reads
      `TF2BDApplication`/`IWorldState`/`IModeratorLogic` directly and mutates `Settings`.
      Keep drawing and interaction logic shared; provide native and HTTP-backed adapters.
- [ ] Move app-only work behind that boundary: scoreboard actions, settings persistence,
      chat-line formatting, setup flow, avatars/textures, filesystem dialogs, and platform
      actions. Browser requests must be asynchronous; the app should validate and dispatch
      mutations on its update thread.
- [ ] Add a localhost HTTP server in `tf2bdapp` to serve the WASM assets, state snapshots,
      and action endpoints. Bind to loopback and protect mutating endpoints from other
      pages/processes; the existing `IHTTPClient` is outbound only.
- [ ] Add an Emscripten SDL2/WebGL build target for the shared ImGui UI, with browser-safe
      font/texture loading and no native OpenGL 4.3/Glad dependency. Test it in the intended
      in-game browser before porting the entire UI.

## Player-list and UI work

- [ ] Build player-list management as new UI work. The abandoned
      `PlayerListManagementWindow` stub (disabled in `DLLMain.cpp`) is reference material,
      not an implementation to finish. Add players by SteamID, edit marks/reasons, and
      remove players from the writable local list without manual JSON edits. Show entries
      from read-only lists without editing those source files.
- [ ] Add a marked-friends detail view: for a selected player, list each marked friend by
      SteamID/name, their marks, and the source list(s). The tooltip currently shows only
      counts by mark; handle private or unavailable friends data explicitly.

## Debugging and tests

- [ ] Make the Debug configuration and `--run-tests` path reliably build and run on supported
      platforms. The existing Catch2 tests are gated by `TF2BD_ENABLE_TESTS` and need a
      repeatable local/CI invocation.
- [ ] Add a reusable fake world/application state with representative players, teams, marks,
      friends, chat, and updates. Use it for deterministic UI development and meaningful
      tests without launching TF2 or requiring a live Steam session. The current test-only
      `DummyWorldState` throws for most operations.

## AppImage target (Linux) — WORKING

A basic AppImage builds and runs (verified locally). Turned out easy: the binary is nearly
self-contained (vcpkg deps static-linked; only `libtbb`/`libstdc++`/`libgcc_s` are private
dynamic deps; SDL2 `dlopen`s host X11/GL/wayland, which we leave to the host).

**Distribution model:** the AppImage is *just the binary*. `cfg/ fonts/ images/ licenses/ logs/
temp/ tf2_addons/` + the `.AppImage` ship together in one folder, like the Windows portable zip —
nothing packaged *inside* the read-only image. `dirname($APPIMAGE)` is the writable portable folder
for both reads and writes; no read/write split, no XDG, no separate data dir.

**Done:**
- [x] **Data-dir fix:** `Platform::GetCurrentExeDir()` (Linux) returns `path($APPIMAGE).parent_path()`
      when `$APPIMAGE` is set, else `/proc/self/exe`'s dir. Single caller (`Filesystem.cpp:73 →
      m_ExeDir`), so the search path / Steam-cwd `current_path()` chdir / `GetLocalAppDataDir` /
      `GetTempDir` all follow. (Also fixed a latent `readlink` non-null-termination bug.)
- [x] **Build script:** `packaging/linux/build-appimage.sh` — self-contained (downloads
      appimagetool, uses committed 256px `packaging/linux/tf2_bot_detector.png` so CI needs no image
      tooling), `ldd`-bundles the private libs (auto-includes `discord_game_sdk.so` if a non-static
      build links it), writes `.desktop` + `AppRun` (sets `SDL_VIDEODRIVER=x11`, no chdir), runs
      appimagetool with `APPIMAGE_EXTRACT_AND_RUN=1` (FUSE-less). Output: `dist/*.AppImage`.
- [x] **CI:** `build-linux.yml` builds the AppImage and uploads `dist/` (with resources copied
      beside the `.AppImage`) as `tf2-bot-detector_appimage_*`.

**Remaining / nice-to-have:**
- [ ] Replace the icon — currently a 32px `.ico` frame upscaled to 256 (blurry); want proper hi-res art.
- [ ] Test on a few distros (older glibc especially) — see if `libstdc++`/`libgcc_s` bundling is
      enough or if more compat work is needed.
- [ ] Discord on Linux: current static build excludes discord (`platform: "!static"`); when shipping
      it, confirm `discord_game_sdk.so` gets bundled (the `ldd` loop already would) and works.
- [ ] Optional ABI hardening: build in the **sniper SDK** (Steam Runtime 3.0, glibc 2.31) for a wide
      floor — guaranteed present post-TF2-x64 (TF2 requires sniper, see `TF2CommandLinePage.cpp:294`).
      Run host-side; do NOT launch TF2BD *through* sniper (TF2BD launches TF2 via sniper → nested
      pressure-vessel).
- [ ] Optional: wire the AppImage build as a CMake target too (not just CI).

## CI

- [x] Removed the stale `submodules/mh_stuff/libmh-stuff.so` staging copy from
      `build-linux.yml`; the build-artifact staging step now copies only the executable.
- [ ] Audit the compiler/runtime Windows minimum before changing `windows-2022` to
      `windows-latest`; dropping cpprestsdk alone does not establish compatibility
      with Windows 8.1 or Windows 7 SP1 on a newer toolset.

## Carried over from migration.md (still open)

- [ ] **Finish dropping the `mh::stuff` shim.** SourceRCON no longer uses mh
      (`locked_value` → `std::mutex` done in fork commit `f0275ed`; its CMake `mh::stuff`
      link + FetchContent block are gone). The launcher and CLI targets still link the root
      `mh_vendored` / `mh::stuff` INTERFACE target; move their required includes/definitions
      to the appropriate target(s) before deleting the shim.
- [ ] **fmt 11/12** — requires bumping the vcpkg submodule + `builtin-baseline` to a 2025+ commit
      (re-resolves all ports). Then drop the `_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING`
      workaround in `tf2_bot_detector_common/CMakeLists.txt`.
- [ ] **Delete the now-dead custom `fmt::formatter<mh::source_location>`** (fmt 10 provides one
      for `char`; ours is a partial spec that's dead for `char`).
- [ ] **Optional: de-`mh`-ify** — move the vendored `mh/` headers under their own namespace if
      full de-mh-ification is wanted (kept `mh::` to minimize churn).

## Done this session (for reference)

- [x] `submodules/mh_stuff` gitlink was already removed; cleaned up its stale `.gitmodules` entry.
- [x] SourceRCON `mh::locked_value<srcon_addr>` → plain value + `std::mutex` (fork `f0275ed`).
- [x] Pruned unused vendored `mh/` headers (deleted 17; closure now 59/59, no dead files).
- [x] CI: pinned Windows runner to `windows-2022`; removed NuGet binary caching from both
      workflows (+ vestigial `VCPKG_CACHE_VERSION`).
- [x] Deleted all 117 stale vcpkg binary-cache nuget packages from GitHub Packages.

## Ideas

- [ ] Copy [NetHook2](https://github.com/SteamRE/SteamKit/tree/master/Resources/NetHook2)'s
      implementation to get a better TF2 game state from packet data.
      Probably too much work to maintain; revisit someday. Assess VAC risk before trying it.
