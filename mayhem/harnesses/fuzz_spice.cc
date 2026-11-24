// SPICE netlist reader harness (db::NetlistSpiceReader, the reader behind Netlist.read / LVS
// "schematic"). .include directives resolve relative to the input's (empty) path, i.e. the cwd.
#include <cstdint>

#include "tlStream.h"
#include "tlException.h"
#include "kl_fuzz.h"
#include "dbNetlist.h"
#include "dbNetlistSpiceReader.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  kl_fuzz_init ();
  tl::InputMemoryStream ms ((const char *) data, size);
  tl::InputStream is (ms);
  db::Netlist nl;
  db::NetlistSpiceReader reader;
  try {
    reader.read (is, nl);
  } KL_CATCH_REJECTED {
  }
  return 0;
}
