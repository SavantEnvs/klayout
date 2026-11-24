// tl::Eval expression language harness: parse an expression (the language of layer-property
// sources, DRC/LVS strings and KLayout's text-substitution "${...}"), then evaluate it.
#include <cstdint>
#include <string>

#include "tlExpression.h"
#include "tlException.h"
#include "kl_fuzz.h"
#include "tlVariant.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  kl_fuzz_init ();
  std::string s ((const char *) data, size);
  tl::Eval eval;
  try {
    tl::Expression expr;
    eval.parse (expr, s);
    tl::Variant v = expr.execute ();
    (void) v.to_string ();
  } KL_CATCH_REJECTED {
  }
  return 0;
}
