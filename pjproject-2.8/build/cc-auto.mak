export CC = arm-oe-linux-gnueabi-gcc  -march=armv7-a -mfloat-abi=softfp -mfpu=neon --sysroot=/home/chenyuxing/mifi-project/cpe-lm321/poky/build/tmp-glibc/sysroots/mdm9607 -c
export CXX = arm-oe-linux-gnueabi-g++  -march=armv7-a -mfloat-abi=softfp -mfpu=neon --sysroot=/home/chenyuxing/mifi-project/cpe-lm321/poky/build/tmp-glibc/sysroots/mdm9607 -c
export AR = arm-oe-linux-gnueabi-ar
export AR_FLAGS = rv
export LD = arm-oe-linux-gnueabi-gcc --sysroot=/home/chenyuxing/mifi-project/cpe-lm321/poky/build/tmp-glibc/sysroots/mdm9607 
export LDOUT = -o 
export RANLIB = arm-oe-linux-gnueabi-ranlib

export OBJEXT := .o
export LIBEXT := .a
export LIBEXT2 := 

export CC_OUT := -o 
export CC_INC := -I
export CC_DEF := -D
export CC_OPTIMIZE := -O2
export CC_LIB := -l

export CC_SOURCES :=
export CC_CFLAGS := -Wall
export CC_LDFLAGS :=

