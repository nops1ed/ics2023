# Project Status

Last updated: 2026-08-18
Status: In progress

## Current focus

Restore a reproducible RISC-V 64 PAL runtime on the historically working `extra` branch, without the ICS course tracer or automatic Git commits.

## Completed

- Switched the local working branch from `master` to `extra`, preserving the tracer-removal changes.
- Confirmed that `extra` contains the PAL-critical loader, user-stack, heap, VME, trap, and device fixes missing from `master`/`pa4`.
- Removed the course tracer integration from the root and NEMU make flows.
- Restored the public `pal-navy` and `newlib-navy` sources through HTTPS.
- Added an RV64 Native ELF NEMU defconfig with devices enabled and tracing disabled.
- Changed Navy dependency bootstrap URLs from SSH to HTTPS and made PAL data an explicit external input.

## In progress

- Build NEMU, PAL, the minimal Navy ramdisk, and nanos-lite in a containerized Ubuntu toolchain.
- Run a headless NEMU smoke test and capture the first runtime failure or successful boot.

## Next

1. Build NEMU with `riscv64-native_defconfig`.
2. Build the minimal `APPS=pal TESTS=hello` ramdisk.
3. Supply a licensed PAL resource directory through `PAL_DATA_DIR`.
4. Run nanos-lite under NEMU, first headlessly and then in a graphical session.

## Blockers

- The proprietary PAL game resource files are not present in the repository, Git history, Git LFS, or the current machine. Upstream intentionally does not distribute them.
- This SSH session has no `DISPLAY` or `WAYLAND_DISPLAY`, so graphical verification requires X11/Wayland forwarding or another graphical session.

## Verification status

- Branch/history audit: passed; `origin/extra` is the strongest PAL recovery baseline.
- External source availability: passed via GitHub HTTPS.
- Static checks: pending final diff review.
- Integration/build: in progress.
- Graphical runtime: blocked by missing licensed data and display access.

## Important context

- The strongest historical PAL run point is `054a4dd3` on `extra`; the current branch tip adds later monitor/ftrace cleanup without replacing the PAL runtime fixes.
- `navy-apps/apps/pal/repo/`, `navy-apps/libs/libc/`, generated ramdisks, and NEMU build outputs are intentionally ignored.
- PAL still launches as `/bin/pal --skip`; `/bin/hello` remains the background process used by the current scheduler.
