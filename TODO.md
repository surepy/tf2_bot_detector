# TODO

Living task list. Historical context for the mh_stuff work lives in `migration.md`.

## Drop cpprestsdk (replace HTTP transport)

**Why:** cpprestsdk is archived by Microsoft and was **removed from vcpkg** upstream
(`9ceec72e0a [cpprestsdk, azure-storage-cpp] Deindex (#52130)`). Our pinned baseline
(`2537044…`, 2022-10-28) still has it at **2.10.18**, which uses `stdext::checked_array_iterator`
— removed in the VS 2026 / MSVC 14.51 toolset, so it fails to compile on newer toolchains
(the reason CI is pinned to `windows-2022`, see below). Upstream did patch it
(`#51750`, in 2.10.19) but then deindexed it, so bumping the baseline is not a durable path.

**Scope is small and isolated** — only `tf2_bot_detector/Networking/HTTPClient.cpp` touches
cpprestsdk. Everything around it is transport-agnostic:
- Public interface `IHTTPClient` (`GetString` / `GetStringAsync → mh::task<std::string>` /
  `GetRequestCounts`) leaks no cpprest types.
- Error types `http_error` / `HTTPResponseCode` / `URL` (`HTTPHelpers.h`) are built on
  `mh::error_condition_exception` + nlohmann — no cpprest.
- ~10 call sites (SteamAPI, LogsTFAPI, GithubAPI, ConfigHelpers, SteamHistoryAPI) only
  `co_await GetStringAsync(...)`. No call-site churn.
- The throttle/retry/counting logic (bulk of HTTPClient.cpp) stays as-is.

cpprest is used only for: per-host `web::http::client::http_client` (cached by
`GetSchemeHostPort`), `request(GET, path)`, `status_code()`, `extract_utf8string`,
`http_exception`, and `utility::conversions::to_string_t`.

**Recommendation: cpp-httplib over libcurl.**
- `HTTPClient.cpp:1-2` already define `CPPHTTPLIB_OPENSSL_SUPPORT` / `CPPHTTPLIB_ZLIB_SUPPORT`,
  and cpp-httplib was already trialed here (it was in the deleted nuget binary cache). It's just
  not in `vcpkg.json` / CMake currently.
- `res->body` is already a UTF-8 `std::string` → deletes every `utility::conversions::to_string_t`
  call (the UTF-16 dance only existed because cpprest uses `wstring` on Windows).
- Header-only, `Get(path)` is one call; no write callbacks / handle lifecycle.
- libcurl is heavier here: write-callback boilerplate, `curl_global_init`, and a `CURL*` easy
  handle can't be shared across threads (breaks the per-host cache + thread-pool offload). No
  functional gain for a GET-only use case.

**Async bridge is already solved:** both libcurl and cpp-httplib are blocking, but
`mh::thread_pool::add_task(fn)` returns an `mh::task<T>` you can `co_await` (runs the blocking
call on a pool thread, resumes the coroutine on completion). This also **deletes the
`#ifdef __linux__ … co_await … #else pplawait …` split** (HTTPClient.cpp:168-182) and `pplawait.h`.

**Steps:**
- [ ] Add cpp-httplib to `vcpkg.json`; swap `find_package(cpprestsdk)` / `cpprestsdk::cpprest`
      in `tf2_bot_detector/CMakeLists.txt:378,392` for cpp-httplib.
- [ ] Rewrite the ~50 transport lines in `HTTPClient.cpp` (cache `httplib::Client` per
      scheme+host+port; `co_await pool.add_task([cli,path]{ return cli->Get(path); })`; map
      `!res`/`res.error()` to the retry path and `res->status` to `http_error`).
- [ ] Remove the `cpprestsdk` dep from `vcpkg.json`.
- [ ] Set `set_connection_timeout` / `set_read_timeout`; add `CPPHTTPLIB_BROTLI_SUPPORT` if brotli
      response decoding parity with cpprest's `compression` feature is wanted.
- [ ] Note: `httplib::Client` serializes requests via an internal socket mutex — fine given
      per-host throttling already mostly serializes, but worth verifying under concurrency.
- [ ] Once done, CI can move back off the `windows-2022` pin to `windows-latest`.

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
- [ ] Optional: wire the AppImage build as a CMake/CI target.

## CI

- [ ] **`build-linux.yml:151`** still copies `submodules/mh_stuff/libmh-stuff.so` to staging —
      stale after the mh_stuff removal (header-only now, no `.so` built). Will fail the Linux
      discord-integration artifact upload. Remove the line (or repoint if a real artifact exists).
- [ ] Move CI Windows runner back to `windows-latest` after cpprestsdk is dropped (currently
      pinned to `windows-2022` to dodge the VS 2026 `stdext` removal).

## Carried over from migration.md (still open)

- [ ] **Finish dropping the `mh::stuff` shim.** SourceRCON no longer uses mh
      (`locked_value` → `std::mutex` done in fork commit `f0275ed`; its CMake `mh::stuff`
      link + FetchContent block are gone). Now delete the `mh_vendored` / `mh::stuff` INTERFACE
      target from the root `CMakeLists.txt` — nothing depends on it anymore.
- [ ] **`git rm` the `submodules/mh_stuff` submodule** and its `.gitmodules` entry. Held back
      until the build is fully green so originals stay available for reference.
- [ ] **fmt 11/12** — requires bumping the vcpkg submodule + `builtin-baseline` to a 2025+ commit
      (re-resolves all ports). Then drop the `_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING`
      workaround in `tf2_bot_detector_common/CMakeLists.txt`.
- [ ] **Delete the now-dead custom `fmt::formatter<mh::source_location>`** (fmt 10 provides one
      for `char`; ours is a partial spec that's dead for `char`).
- [ ] **Optional: de-`mh`-ify** — move the vendored `mh/` headers under their own namespace if
      full de-mh-ification is wanted (kept `mh::` to minimize churn).

## Done this session (for reference)

- [x] SourceRCON `mh::locked_value<srcon_addr>` → plain value + `std::mutex` (fork `f0275ed`).
- [x] Pruned unused vendored `mh/` headers (deleted 17; closure now 59/59, no dead files).
- [x] CI: pinned Windows runner to `windows-2022`; removed NuGet binary caching from both
      workflows (+ vestigial `VCPKG_CACHE_VERSION`).
- [x] Deleted all 117 stale vcpkg binary-cache nuget packages from GitHub Packages.
