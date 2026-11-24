// Report database reader harness: rdb::Reader autodetects KLayout's XML .lyrdb and Calibre RVE
// result files and fills an rdb::Database (the marker browser's input).
#include <cstdint>

#include "tlStream.h"
#include "tlException.h"
#include "kl_fuzz.h"
#include "rdb.h"
#include "rdbReader.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  kl_fuzz_init ();
  tl::InputMemoryStream ms ((const char *) data, size);
  tl::InputStream is (ms);
  rdb::Database db;
  try {
    rdb::Reader reader (is);
    reader.read (db);
  } KL_CATCH_REJECTED {
  }
  return 0;
}
