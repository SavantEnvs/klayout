// Layout stream-format reader harness. One source, one binary per format: the format is chosen at
// compile time with -DKL_FORMAT_<NAME>. Each binary feeds the input through the format's own reader
// class (what db::Reader instantiates after format detection) into a fresh db::Layout.
//
// Formats whose reader picks behaviour from the file name (LEF vs DEF) get the input as a file with
// the right suffix in a private scratch directory under $TMPDIR.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

#include "tlStream.h"
#include "tlException.h"
#include "kl_fuzz.h"
#include "dbLayout.h"
#include "dbLoadLayoutOptions.h"
#include "dbReader.h"

#if defined(KL_FORMAT_GDS2)
#  include "dbGDS2Reader.h"
typedef db::GDS2Reader reader_t;
#elif defined(KL_FORMAT_OASIS)
#  include "dbOASISReader.h"
typedef db::OASISReader reader_t;
#elif defined(KL_FORMAT_DXF)
#  include "dbDXFReader.h"
typedef db::DXFReader reader_t;
#elif defined(KL_FORMAT_CIF)
#  include "dbCIFReader.h"
typedef db::CIFReader reader_t;
#elif defined(KL_FORMAT_MAG)
#  include "dbMAGReader.h"
typedef db::MAGReader reader_t;
#elif defined(KL_FORMAT_MALY)
#  include "dbMALYReader.h"
typedef db::MALYReader reader_t;
#elif defined(KL_FORMAT_GDS2TXT)
#  include "dbGDS2TextReader.h"
typedef db::GDS2ReaderText reader_t;
#elif defined(KL_FORMAT_LEF) || defined(KL_FORMAT_DEF)
#  include "dbLEFDEFImporter.h"
typedef db::LEFDEFReader reader_t;
#  define KL_VIA_FILE 1
#  if defined(KL_FORMAT_LEF)
#    define KL_SUFFIX ".lef"
#  else
#    define KL_SUFFIX ".def"
#  endif
#else
#  error "define one KL_FORMAT_*"
#endif

static void run (tl::InputStream &is)
{
  db::Layout layout;
  reader_t reader (is);
  try {
    reader.read (layout, db::LoadLayoutOptions ());
#if defined(KL_FORMAT_DXF)
  } catch (tl::InternalException &ex) {
    //  Known upstream bug, filtered so the target can fuzz at all: DXFReader::prepare_read
    //  (dbDXFReader.cc) sniffs the binary header with m_stream.get (22) and calls
    //  m_stream.unget (22) even when get() returned 0, so every input shorter than 22 bytes
    //  (including libFuzzer's initial empty input) trips tl_assert (mp_buffer + n <= mp_bptr) in
    //  tl::InputStream::unget (tlStream.cc). Every other assertion still aborts.
    if (ex.msg ().find ("tlStream.cc") == std::string::npos ||
        ex.msg ().find ("mp_buffer + n <= mp_bptr") == std::string::npos) {
      kl_fuzz_internal_error (ex);
    }
#endif
  } KL_CATCH_REJECTED {
    //  rejected input: the reader's documented error path
  }
}

#if defined(KL_VIA_FILE)
static std::string s_dir;

extern "C" int LLVMFuzzerInitialize (int *, char ***)
{
  const char *t = getenv ("TMPDIR");
  std::string tmpl = std::string (t && *t ? t : "/tmp") + "/kl_fuzz_XXXXXX";
  char *d = mkdtemp (&tmpl[0]);
  if (! d) {
    abort ();
  }
  s_dir = d;
  return 0;
}
#endif

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  kl_fuzz_init ();
#if defined(KL_VIA_FILE)
  if (s_dir.empty ()) {
    LLVMFuzzerInitialize (0, 0);
  }
  std::string path = s_dir + "/input" KL_SUFFIX;
  FILE *f = fopen (path.c_str (), "wb");
  if (! f) {
    abort ();
  }
  if (size > 0) {
    fwrite (data, 1, size, f);
  }
  fclose (f);
  {
    tl::InputStream is (path);
    run (is);
  }
  unlink (path.c_str ());
#else
  tl::InputMemoryStream ms ((const char *) data, size);
  tl::InputStream is (ms);
  run (is);
#endif
  return 0;
}
