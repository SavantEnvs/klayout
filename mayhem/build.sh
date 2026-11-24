#!/usr/bin/env bash
#
# mayhem/build.sh — build KLayout's core libraries (tl, gsi, db, rdb) and stream-format plugins
# Qt-less and statically (mayhem/klayout.mk), the libFuzzer harnesses in mayhem/harnesses/ plus a
# -standalone reproducer for each, and the tl unit-test runner (normal flags) that test.sh runs.
# Re-runnable: make only rebuilds what changed; nothing is fetched.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS SRC

cd "$SRC"
B="$SRC/mayhem/build"
FUZZ_OUT="$B/fuzz"
TEST_OUT="$B/test"
mkdir -p "$B"

# make does not track compiler flags: start a tree over when they change (e.g. a different
# --build-arg SANITIZER_FLAGS), so objects never mix flag sets.
flags_stamp="$CXX|$SANITIZER_FLAGS|$DEBUG_FLAGS|$COVERAGE_FLAGS"
for d in "$FUZZ_OUT" "$TEST_OUT"; do
  if [ -d "$d" ] && [ "$(cat "$d/.flags" 2>/dev/null)" != "$flags_stamp" ]; then rm -rf "$d"; fi
  mkdir -p "$d"; printf '%s' "$flags_stamp" > "$d/.flags"
done

# UBSan's alignment check is relaxed (-fno-sanitize=alignment): the GDS2 reader reads every record
# through uint32_t/double casts of an unaligned byte buffer (dbGDS2Reader.cc get_int/get_double), which
# aborts on the first record of every valid file. ASan and the rest of UBSan stay on and halting.
SANITIZER_FLAGS="$SANITIZER_FLAGS${SANITIZER_FLAGS:+ -fno-sanitize=alignment}"

# 1) The project, sanitized. The libraries carry line tables only (-gline-tables-only, still DWARF 3):
#    with full -g every harness binary is ~400 MB; line tables keep file:line in every backtrace.
#    -fsanitize=fuzzer-no-link adds the coverage instrumentation libFuzzer needs to the library code.
make -f mayhem/klayout.mk -j"$MAYHEM_JOBS" SRC="$SRC" OUT="$FUZZ_OUT" CXX="$CXX" \
  CXXFLAGS_EXTRA="$SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS -gline-tables-only" all

KL_INC="-I$SRC/src/tl/tl -I$SRC/src/gsi/gsi -I$SRC/src/db/db -I$SRC/src/rdb/rdb -I$SRC/src/plugins/common"
for p in gds2 oasis dxf cif magic maly lefdef pcb; do
  KL_INC="$KL_INC -I$SRC/src/plugins/streamers/$p/db_plugin"
done
KL_INC="$KL_INC -I$SRC/src/plugins/streamers/gds2/db_plugin/contrib"
KL_DEFS="-DHAVE_EXPAT=1 -DHAVE_PTHREADS=1"
# The plugins register their stream formats and technology components through static objects, so
# their archives (and rdb's report formats, gsi's and db's script class declarations) are linked whole; the rest is pulled in by reference.
KL_LIBS="-Wl,--whole-archive $FUZZ_OUT/libkl_plugins.a $FUZZ_OUT/libkl_rdb.a $FUZZ_OUT/libkl_gsi.a $FUZZ_OUT/libkl_dbgsi.a -Wl,--no-whole-archive $FUZZ_OUT/libkl_db.a $FUZZ_OUT/libkl_tl.a -lexpat -lz -lpthread -ldl"
HFLAGS="-std=c++17 -O1 $SANITIZER_FLAGS $DEBUG_FLAGS $KL_DEFS $KL_INC"

# 2) Harnesses: libFuzzer binary + standalone reproducer each.
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$B/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$B/standalone_main.o"
$CXX $HFLAGS -c "$SRC/mayhem/harnesses/kl_init.cc" -o "$B/kl_init.o"

build_harness() {  # <binary name> <source> [extra flags...]
  local name="$1" src="$2"; shift 2
  $CXX $HFLAGS "$@" -c "$src" -o "$B/$name.o"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE -fuse-ld=lld "$B/$name.o" "$B/kl_init.o" "$B/lsan_off.o" $KL_LIBS -o "$SRC/$name"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS -fuse-ld=lld "$B/$name.o" "$B/kl_init.o" "$B/standalone_main.o" "$B/lsan_off.o" $KL_LIBS -o "$SRC/$name-standalone"
}

H="$SRC/mayhem/harnesses"
for f in GDS2 OASIS DXF CIF MAG MALY GDS2TXT LEF DEF; do
  build_harness "fuzz_$(echo "$f" | tr 'A-Z' 'a-z')" "$H/fuzz_stream_reader.cc" "-DKL_FORMAT_$f"
done
for h in gerber spice l2n rdb expression layer_map technology; do
  build_harness "fuzz_$h" "$H/fuzz_$h.cc"
done

# 3) The tl unit tests (upstream's src/tl/unit_tests), normal flags, run by test.sh.
make -f mayhem/klayout.mk -j"$MAYHEM_JOBS" SRC="$SRC" OUT="$TEST_OUT" CXX="$CXX" \
  CXXFLAGS_EXTRA="-O2 $COVERAGE_FLAGS" tl
TL_TESTS="tlAlgorithmTests tlBase64Tests tlExpressionTests tlStringTests tlVariantTests tlKDTreeTests
  tlIntervalMapTests tlIntervalSetTests tlLongIntTests tlInt128SupportTests tlGlobPatternTests tlUriTests
  tlMathTests tlReuseVectorTests tlStableVectorTests tlListTests tlSListTests tlOptionalTests
  tlDeflateTests tlXMLParserTests tlStreamTests tlEquivalenceClustersTests tlObjectTests tlEventsTests"
TOBJS=""
mkdir -p "$TEST_OUT/ut"
for t in $TL_TESTS; do
  $CXX -std=c++17 -O2 $COVERAGE_FLAGS $KL_DEFS -I"$SRC/src/tl/tl" -I"$SRC/src/tl/unit_tests" \
    -c "$SRC/src/tl/unit_tests/$t.cc" -o "$TEST_OUT/ut/$t.o"
  TOBJS="$TOBJS $TEST_OUT/ut/$t.o"
done
$CXX -std=c++17 -O2 $COVERAGE_FLAGS -I"$SRC/src/tl/tl" -c "$SRC/mayhem/tl_test_main.cc" -o "$TEST_OUT/ut/main.o"
$CXX $COVERAGE_FLAGS "$TEST_OUT/ut/main.o" -Wl,--whole-archive $TOBJS -Wl,--no-whole-archive \
  "$TEST_OUT/libkl_tl.a" -lexpat -lz -lpthread -ldl -o "$B/tl_unit_tests"
echo "build.sh: done"
