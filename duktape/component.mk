COMPONENT_ADD_INCLUDEDIRS := src
COMPONENT_OBJS := src/duktape.o
COMPONENT_SRCDIRS := . src extras extras/print-alert extras/console

#-std=gnu99 -Os -ggdb -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -mlongcalls -nostdlib -Wall -Werror=all -Wno-error=unused-function -Wno-error=unused-but-set-variable -Wno-error=unused-variable -Wno-error=deprecated-declarations -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-old-style-declaration -O3 -DESP_PLATFORM -D IDF_VER=\"v3.2.2-dirty\" -MMD -MP 
CFLAGS := -DCONFIG_VERSION=\"2019-07-21\" -D_ESP32 -O3 -ggdb -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -mlongcalls -nostdlib -Wall -Werror=all -Wno-error=unused-function -Wno-error=unused-but-set-variable -Wno-error=unused-variable -Wno-error=deprecated-declarations -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-old-style-declaration -DESP_PLATFORM -D IDF_VER=\"v3.2.2-dirty\" -MMD -MP 