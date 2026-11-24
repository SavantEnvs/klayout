// KLayout netlist database reader harness: .l2n (LayoutToNetlist standard format) and .lvsdb
// (LayoutVsSchematic standard format). The first byte picks the reader (even: l2n, odd: lvsdb).
#include <cstdint>

#include "tlStream.h"
#include "tlException.h"
#include "kl_fuzz.h"
#include "dbLayoutToNetlist.h"
#include "dbLayoutToNetlistReader.h"
#include "dbLayoutVsSchematic.h"
#include "dbLayoutVsSchematicReader.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  kl_fuzz_init ();
  if (size < 1) {
    return -1;
  }
  bool lvs = (data[0] & 1) != 0;
  tl::InputMemoryStream ms ((const char *) data + 1, size - 1);
  tl::InputStream is (ms);
  try {
    if (lvs) {
      db::LayoutVsSchematic db;
      db::LayoutVsSchematicStandardReader r (is);
      r.read (&db);
    } else {
      db::LayoutToNetlist db;
      db::LayoutToNetlistStandardReader r (is);
      r.read (&db);
    }
  } KL_CATCH_REJECTED {
  }
  return 0;
}
