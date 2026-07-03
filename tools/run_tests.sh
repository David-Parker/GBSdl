#!/bin/sh
# Regression suite for GBLib, run through the GBHeadless harness.
#
# Usage: tools/run_tests.sh [build-dir]
#
# Serial-reporting tests (cpu_instrs) pass/fail automatically. Screen-reporting
# tests (halt_bug, instr_timing, mem_timing) and the Pokemon Crystal warp
# regression write screenshots to <build-dir>/test-out for visual inspection.
#
# Expected results: cpu_instrs, instr_timing, halt_bug pass. mem_timing fails
# 3 tests (requires sub-instruction memory access timing, which this emulator
# does not model). crystal_warp.script should end on the house 1F interior
# with Mom talking to the player -- a blank/cream screen means the LCD-off or
# interrupt regressions are back.

set -e

BUILD="${1:-build}"
OUT="$BUILD/test-out"
mkdir -p "$OUT"

cd "$BUILD"

echo "--- cpu_instrs (CGB) ---"
./GBHeadless ./rom cpu_instrs.gb --frames 4200 | grep -E "Passed|Failed"

echo "--- cpu_instrs (DMG) ---"
./GBHeadless ./rom cpu_instrs.gb --frames 4200 --emutype dmg | grep -E "Passed|Failed"

echo "--- halt_bug (see test-out/halt_bug.bmp) ---"
./GBHeadless ./rom gb-test-roms/halt_bug.gb \
    --script ../tools/scripts/halt_bug.script --out test-out > /dev/null

echo "--- instr_timing (see test-out/instr_timing.bmp) ---"
./GBHeadless ./rom gb-test-roms/instr_timing/instr_timing.gb \
    --script ../tools/scripts/instr_timing.script --out test-out > /dev/null

echo "--- mem_timing (see test-out/mem_timing.bmp; 3 known failures) ---"
./GBHeadless ./rom gb-test-roms/mem_timing/mem_timing.gb \
    --script ../tools/scripts/mem_timing.script --out test-out > /dev/null

if [ -f rom/Pokemon-Crystal.gbc ]; then
    echo "--- Pokemon Crystal warp regression (see test-out/crystal_warp.bmp) ---"

    # The script drives the new-game path, so stash any battery save (which
    # would change the title menu to CONTINUE) and restore it afterwards.
    SAVE=rom/saves/PM_CRYSTAL.save
    if [ -f "$SAVE" ]; then
        mv "$SAVE" "$SAVE.stash"
        trap 'mv "$SAVE.stash" "$SAVE" 2>/dev/null' EXIT
    fi

    ./GBHeadless ./rom Pokemon-Crystal.gbc \
        --script ../tools/scripts/crystal_warp.script --out test-out | grep -E "Frames|ERROR"

    if [ -f "$SAVE.stash" ]; then
        mv "$SAVE.stash" "$SAVE"
        trap - EXIT
    fi
else
    echo "--- Pokemon Crystal not present; skipping warp regression ---"
fi

echo "Done. Screenshots in $OUT"
