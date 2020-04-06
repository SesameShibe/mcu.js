#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := mcujs
EXTRA_COMPONENT_DIRS := ../../duktape

include $(IDF_PATH)/make/project.mk

flash_font: ../../font/font.bin
	python2 $(IDF_PATH)/components/partition_table/parttool.py --port COM6 -n font write_partition --input $<

../../font/font.bin:
	python2 ../../py/font.py -r -t ../../fonts/SourceHanSansSC-ExtraLight.otf -l 79 -p 0 -b 2 -o $@