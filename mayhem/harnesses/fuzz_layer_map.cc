// String parsers behind reader options: db::LayerMap::from_string_file_format (layer map files,
// "-m" of the strm* tools, technology reader options), db::LayerProperties::read and the
// tl::from_string number parsers they use.
#include <cstdint>
#include <string>

#include "tlString.h"
#include "tlException.h"
#include "kl_fuzz.h"
#include "dbStreamLayers.h"
#include "dbLayerProperties.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  kl_fuzz_init ();
  std::string s ((const char *) data, size);
  try {
    db::LayerMap lm = db::LayerMap::from_string_file_format (s);
    (void) lm.to_string_file_format ();
  } KL_CATCH_REJECTED {
  }
  try {
    db::LayerProperties lp;
    tl::Extractor ex (s.c_str ());
    lp.read (ex);
    (void) lp.to_string ();
  } KL_CATCH_REJECTED {
  }
  try { int v; tl::from_string (s, v); } KL_CATCH_REJECTED { }
  try { long v; tl::from_string (s, v); } KL_CATCH_REJECTED { }
  try { unsigned long v; tl::from_string (s, v); } KL_CATCH_REJECTED { }
  try { long long v; tl::from_string (s, v); } KL_CATCH_REJECTED { }
  try { double v; tl::from_string (s, v); } KL_CATCH_REJECTED { }
  return 0;
}
