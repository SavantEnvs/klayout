// Gerber RS274X / Excellon drill reader harness: the readers db::GerberImporter dispatches to for
// each file of a PCB project. The first byte picks the reader (even: RS274X, odd: drill file);
// the rest is the file content.
#include <cstdint>
#include <string>
#include <vector>

#include "tlStream.h"
#include "tlException.h"
#include "kl_fuzz.h"
#include "dbLayout.h"
#include "dbRS274XReader.h"
#include "dbGerberDrillFileReader.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  kl_fuzz_init ();
  if (size < 1) {
    return -1;
  }
  bool drill = (data[0] & 1) != 0;
  tl::InputMemoryStream ms ((const char *) data + 1, size - 1);
  tl::InputStream is (ms);
  tl::TextInputStream ts (is);

  db::Layout layout;
  db::Cell &cell = layout.cell (layout.add_cell ("TOP"));
  std::vector<unsigned int> targets;
  targets.push_back (layout.insert_layer (db::LayerProperties (1, 0)));

  try {
    if (drill) {
      db::GerberDrillFileReader r (0);
      r.read (ts, layout, cell, targets);
    } else {
      db::RS274XReader r (0);
      r.read (ts, layout, cell, targets);
    }
  } KL_CATCH_REJECTED {
  }
  return 0;
}
