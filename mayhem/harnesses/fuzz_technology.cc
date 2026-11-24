// Technology file (.lyt) harness: the XML structure db::Technology::load parses (expat), including
// every registered stream format's reader/writer option section, read from memory instead of a file.
#include <cstdint>
#include <string>

#include "tlException.h"
#include "kl_fuzz.h"
#include "tlXMLParser.h"
#include "dbTechnology.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  kl_fuzz_init ();
  std::string s ((const char *) data, size);
  db::Technology t;
  try {
    tl::XMLStringSource source (s);
    tl::XMLStruct<db::Technology> xml_struct ("technology", db::Technology::xml_elements ());
    xml_struct.parse (source, t);
  } KL_CATCH_REJECTED {
  }
  return 0;
}
