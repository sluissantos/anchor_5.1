#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)
#include $(IDF_PATH)/make/component_common.mk
#COMPONENT_ADD_INCLUDEDIRS := .
#COMPONENT_ADD_LDFLAGS +=$(COMPONENT_PATH)/lib/lib.a
COMPONENT_SRCDIRS := . dwm_driver/dwm_api/lmh dwm_driver/dwm_api platform/esp/hal
COMPONENT_ADD_INCLUDEDIRS := . include/ dwm_driver/dwm_api/lmh/ platform/esp/hal/