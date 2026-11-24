# mayhem/klayout.mk — Qt-less static build of KLayout's core libraries (tl, gsi, db, rdb) and the
# stream-format reader plugins, for the fuzz harnesses. Upstream builds with qmake into shared
# libraries + dlopen'ed plugins; we compile the same sources (as setup.py does for the Python module:
# "every *.cc in the module dir") into static archives so the harnesses link them in-process.
#
# Variables: OUT (object dir), CXX, CXXFLAGS_EXTRA (sanitizer/debug flags), VARIANT_LIBS (which libs).
# The readers need the scripting class declarations (gsiDecl*.cc) of gsi and db: tl::Variant looks user
# classes up in the registry gsi::initialize() fills (by name in tl::Extractor's variant reader, by C++
# type in tl::Variant (db::Box) etc.) and asserts they are registered. db's go into their own archive,
# libkl_dbgsi.a, that the harnesses link whole. The rdb/plugin gsiDecl*.cc only add script methods
# and are left out.
SRC      ?= /mayhem
OUT      ?= $(SRC)/mayhem/build/fuzz
CXX      ?= clang++
CXXFLAGS_EXTRA ?=

PLUGINS  := gds2 oasis dxf cif magic maly lefdef pcb

INCLUDES := -I$(SRC)/src/tl/tl -I$(SRC)/src/gsi/gsi -I$(SRC)/src/db/db -I$(SRC)/src/rdb/rdb \
            -I$(SRC)/src/plugins/common -I$(SRC)/src/version \
            $(foreach p,$(PLUGINS),-I$(SRC)/src/plugins/streamers/$(p)/db_plugin) \
            -I$(SRC)/src/plugins/streamers/gds2/db_plugin/contrib
DEFINES  := -DHAVE_EXPAT=1 -DHAVE_PTHREADS=1 -DKLAYOUT_VERSION=0.30.14 -DKLAYOUT_MAJOR_VERSION=0 \
            -DKLAYOUT_MINOR_VERSION=30 -DKLAYOUT_TINY_VERSION=14 \
            -DMAKE_TL_LIBRARY -DMAKE_GSI_LIBRARY -DMAKE_DB_LIBRARY -DMAKE_RDB_LIBRARY -DMAKE_DB_PLUGIN_LIBRARY
CXXFLAGS := -std=c++17 -O1 -Wno-deprecated-declarations $(DEFINES) $(INCLUDES) $(CXXFLAGS_EXTRA)

nogsi = $(filter-out %/gsiDecl%.cc,$(wildcard $(1)/*.cc))

# tlWebDAV needs curl or Qt (neither is built in)
TL_SRC   := $(filter-out %/tlWebDAV.cc,$(wildcard $(SRC)/src/tl/tl/*.cc))
GSI_SRC  := $(wildcard $(SRC)/src/gsi/gsi/*.cc)
DB_SRC   := $(call nogsi,$(SRC)/src/db/db)
DBGSI_SRC := $(wildcard $(SRC)/src/db/db/gsiDecl*.cc)
RDB_SRC  := $(call nogsi,$(SRC)/src/rdb/rdb)
PLG_SRC  := $(foreach p,$(PLUGINS),$(call nogsi,$(SRC)/src/plugins/streamers/$(p)/db_plugin)) \
            $(call nogsi,$(SRC)/src/plugins/streamers/gds2/db_plugin/contrib)

obj = $(patsubst $(SRC)/%.cc,$(OUT)/%.o,$(1))

TL_OBJ  := $(call obj,$(TL_SRC))
GSI_OBJ := $(call obj,$(GSI_SRC))
DB_OBJ  := $(call obj,$(DB_SRC))
DBGSI_OBJ := $(call obj,$(DBGSI_SRC))
RDB_OBJ := $(call obj,$(RDB_SRC))
PLG_OBJ := $(call obj,$(PLG_SRC))

.PHONY: all tl
all: $(OUT)/libkl_tl.a $(OUT)/libkl_gsi.a $(OUT)/libkl_db.a $(OUT)/libkl_dbgsi.a $(OUT)/libkl_rdb.a $(OUT)/libkl_plugins.a
tl: $(OUT)/libkl_tl.a

$(OUT)/%.o: $(SRC)/%.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OUT)/libkl_tl.a: $(TL_OBJ)
$(OUT)/libkl_gsi.a: $(GSI_OBJ)
$(OUT)/libkl_db.a: $(DB_OBJ)
$(OUT)/libkl_dbgsi.a: $(DBGSI_OBJ)
$(OUT)/libkl_rdb.a: $(RDB_OBJ)
$(OUT)/libkl_plugins.a: $(PLG_OBJ)
$(OUT)/libkl_%.a:
	rm -f $@
	$(AR) rcs $@ $^

-include $(shell find $(OUT) -name '*.d' 2>/dev/null)
