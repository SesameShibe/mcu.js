#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)
COMPONENT_SRCDIRS := . __generated
COMPONENT_EXTRA_INCLUDES := ../../../../duktape
COMPONENT_EXTRA_INCLUDES += ../../main