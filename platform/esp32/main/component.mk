#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)
COMPONENT_SRCDIRS := . __generated TFT_eSPI
COMPONENT_EXTRA_INCLUDES := ../../../../duktape
COMPONENT_EXTRA_INCLUDES += ../../main ../../main/TFT_eSPI