// Shared by every harness. tl_assert() throws tl::InternalException, which derives from
// tl::Exception: catching only tl::Exception would swallow internal-invariant violations as if
// they were rejected input. An InternalException is a bug, so it aborts (with its message).
#ifndef KL_FUZZ_H
#define KL_FUZZ_H

#include <cstdio>
#include <cstdlib>

#include "tlException.h"

//  Once per process, before the first input is parsed (kl_init.cc). Every LLVMFuzzerTestOneInput
//  calls it first.
void kl_fuzz_init ();

[[noreturn]] inline void kl_fuzz_internal_error (const tl::InternalException &ex)
{
  fprintf (stderr, "tl_assert failed: %s\n", ex.msg ().c_str ());
  abort ();
}

//  Use in place of "catch (tl::Exception &)": rejected input is ignored, assertions abort.
#define KL_CATCH_REJECTED \
  catch (tl::InternalException &kl_ex_) { kl_fuzz_internal_error (kl_ex_); } \
  catch (tl::Exception &)

#endif
