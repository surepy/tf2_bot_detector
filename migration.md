# Migration: removing the `mh_stuff` submodule

## Why

`mh_stuff` (the `submodules/mh_stuff` fork) was an unmaintained "stuff I keep rewriting"
utility library. Its core problem: `mh/text/format.hpp` is a *swappable formatting backend*
(fmt / std::format / boost) that wraps `fmt` so tightly that:

- fmt could not be bumped past **8.1.1** — fmt 9.0.0 changed `make_format_args` / `format_string`
  in ways that broke the wrapper's `mh::format` / `mh::make_format_args` / `check_type` machinery.
- anything **newer than gcc 13** refused to compile the library.
- there was no upside to maintaining a whole abstraction layer for one forked project, since
  we only ever use `fmt` directly anyway (and `fmt::format` gives compile-time format checks).

## Strategy

1. **Delete the one true wrapper** (`mh/text/format.hpp`) and call `fmt::` directly everywhere.
2. **Vendor everything else** (the real utilities — coroutines, `expected`, `enum_fmt`
   reflection, `fmtstr`, `case_insensitive_string`, `from_chars`, etc.) into the repo,
   **keeping the `mh::` namespace and `<mh/...>` include paths** so call sites don't churn.
   We now own the files + compiler flags instead of mh's CMake.

## What changed

### Vendored headers
- The `mh/` include tree was copied from `submodules/mh_stuff/cpp/include/mh` into
  **`tf2_bot_detector_common/include/mh/`** and is now **header-only**
  (`MH_STUFF_API=` and `MH_COMPILE_LIBRARY_INLINE=inline` are defined by CMake).
- `mh/text/format.hpp` was **deleted** (the swappable-backend wrapper, root cause of the fmt pin).
- The only 4 headers that were coupled to that wrapper were rewritten to use `fmt::` directly,
  as idiomatic `fmt::formatter` specializations with `const`-qualified `format()`
  (forward-compatible with fmt 9/10+):
  - `mh/text/fmtstr.hpp`
  - `mh/reflection/enum.hpp`        (`mh::enum_fmt`, supports `{}`, `{:v}`, `{:t}`/`{:T}`)
  - `mh/source_location.hpp`
  - `mh/text/formatters/error_code.hpp`

  > Note: `mh::enum_fmt(x)` and the `MH_ENUM_REFLECT_*` macros were intentionally **kept**.
  > Only the formatter body was repointed at fmt. (A fully-idiomatic per-enum
  > `fmt::formatter` rewrite is possible later, but would touch every reflect block + call site.)

### Call sites
- `#include <mh/text/format.hpp>` → `#include <fmt/format.h>` (42 files).
- `mh::format` / `mh::format_to` / `mh::runtime` / `mh::try_format` → `fmt::` equivalents.
  (`mh::try_format` had no fmt equivalent; the one fatal-error log path now does its own
  try/catch, and `Log.h` has a small local `try_format` helper for compile-time format strings.)

### Build system
- `CMakeLists.txt` (root): removed `set(MH_STUFF_BUILD_SHARED_LIBS ON)` and
  `add_subdirectory(submodules/mh_stuff)`.
- `tf2_bot_detector_common/CMakeLists.txt`: dropped the `mh::stuff` link; added the
  `MH_STUFF_API=` / `MH_COMPILE_LIBRARY_INLINE=inline` PUBLIC compile definitions.
- `tf2_bot_detector_renderer/CMakeLists.txt`: dropped the (already stale) `mh::stuff` link.
- `vcpkg.json`: removed `mh-cmake-common` (only mh_stuff's CMake used it).

### The `mh::stuff` shim (root `CMakeLists.txt`)
`submodules/SourceRCON` still links `mh::stuff` and does
`if (NOT TARGET mh::stuff) FetchContent ... PazerOP/stuff`. Previously our
`add_subdirectory(mh_stuff)` defined that target first so the fetch never ran. With the
submodule gone, SourceRCON would fetch upstream `PazerOP/stuff` (needs CURL + mh-cmake-common)
and fail to configure.

Fix: a tiny **`mh::stuff` INTERFACE target** (`mh_vendored`) is defined *before*
`add_subdirectory(submodules/SourceRCON)`, pointing at the vendored headers. SourceRCON sees
the target already exists, skips the fetch, and compiles against the vendored `mh/`.

## Follow-ups (not done yet)

- [ ] **Drop the `mh::stuff` shim entirely.** SourceRCON's *only* mh usage is
  `mh::locked_value<srcon_addr>` in `src/async_client.cpp` — just a mutex + value wrapper.
  Replace it with a plain `std::mutex`-guarded value (in the surepy/tf2bd_SourceRCON fork),
  then delete `mh_vendored`/`mh::stuff` from the root CMakeLists.
- [ ] **`git rm` the `submodules/mh_stuff` submodule** (and its `.gitmodules` entry). Held back
  until the build is fully green so the originals stay available for reference.
- [ ] **Prune unused vendored headers.** The whole `mh/` tree was copied to get green with low
  risk; headers that nothing includes can be deleted later.
- [ ] **Bump fmt** past 8.1.1 in `vcpkg.json` now that the wrapper is gone (and drop the
  `_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING` workaround in
  `tf2_bot_detector_common/CMakeLists.txt` once on fmt 10.1.1).
- [ ] Consider eventually moving the vendored `mh/` headers under their own namespace if full
  de-`mh`-ification is wanted (kept `mh::` here to minimize churn).
