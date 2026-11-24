// Minimal runner for KLayout's own tl unit tests (src/tl/unit_tests/*Tests.cc). Upstream's runner
// (src/unit_tests/unit_test_main.cc) links the whole application (lay, Ruby, Python); this one only
// runs the registered tl::TestBase instances and prints a parseable summary.
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

#include "tlUnitTest.h"
#include "tlException.h"

int main (int argc, char **argv)
{
  int passed = 0, failed = 0, skipped = 0;
  //  KL_TEST_SKIP: space-separated test names not run here (they need the gsi class registry,
  //  which this tl-only runner does not link).
  std::string skip = std::string (" ") + (getenv ("KL_TEST_SKIP") ? getenv ("KL_TEST_SKIP") : "") + " ";
  const std::vector<tl::TestBase *> &tests = tl::TestRegistrar::instance ()->tests ();
  for (std::vector<tl::TestBase *>::const_iterator t = tests.begin (); t != tests.end (); ++t) {
    std::string name = (*t)->name ();
    if (argc > 1 && name.find (argv[1]) == std::string::npos) {
      continue;
    }
    if (skip.find (" " + name + " ") != std::string::npos) {
      printf ("SKIP %s\n", name.c_str ());
      ++skipped;
      continue;
    }
    bool ok = false;
    try {
      ok = (*t)->do_test (false, false);
    } catch (tl::Exception &ex) {
      fprintf (stderr, "%s: exception: %s\n", name.c_str (), ex.msg ().c_str ());
      ok = false;
    } catch (...) {
      ok = false;
    }
    printf ("%s %s\n", ok ? "PASS" : "FAIL", name.c_str ());
    if (ok) { ++passed; } else { ++failed; }
  }
  printf ("SUMMARY passed=%d failed=%d skipped=%d\n", passed, failed, skipped);
  return failed == 0 && passed > 0 ? 0 : 1;
}
