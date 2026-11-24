// Linked into every harness: kl_fuzz_init() runs gsi::initialize() once, on the first input, as
// KLayout's application and Python module do after all libraries are loaded. That fills the
// tl::Variant user-class registry (by name for tl::Extractor's "[Class:...]" reader, by type for
// tl::Variant (db::Box) and friends), which the readers assert is populated.
//
// It must not run from a namespace-scope static constructor: kl_init.o is linked before the
// archives, so such a constructor runs before the gsi::Class objects are constructed, finds no
// classes, and leaves the registry empty.
#include "gsi.h"
#include "kl_fuzz.h"

void kl_fuzz_init ()
{
  static bool s_done = false;
  if (! s_done) {
    gsi::initialize ();
    s_done = true;
  }
}
