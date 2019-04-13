HOST_PREFIX=x86_64-unknown-linux-gnu
#HOST_PREFIX=arm-oe-linux-gnueabi
#HOST_HOST=arm-oe-linux-gnueabi
CC=gcc

natcheck: pjproject
	${CC} -Wall -o natcheck src/main.c -g -O2 -DPJ_AUTOCONF=1 -I./pjproject-2.8/output/include -L./pjproject-2.8/output/lib -lpjnath-${HOST_PREFIX} -lpjlib-util-${HOST_PREFIX} -lpj-${HOST_PREFIX} -lssl -lcrypto -luuid -lm -lrt -lpthread 

.PHONY:pjproject
pjproject:
	cd pjproject-2.8 && ./configure --prefix=${PWD}/pjproject-2.8/output --host=${HOST_HOST} --disable-resample --disable-sound --disable-video --disable-speex-aec --disable-g711-codec --disable-l16-codec --disable-gsm-codec --disable-g722-codec --disable-g7221-codec --disable-speex-codec --disable-ilbc-codec --disable-sdl --disable-ffmpeg --disable-v4l2 --disable-openh264 --disable-opencore-amr  --disable-silk --disable-opus --disable-bcg729 --disable-libyuv --disable-libwebrtc
	cd pjproject-2.8 && make dep
	cd pjproject-2.8 && make 
	cd pjproject-2.8 && make install

	
clean_nc:
	- @rm natcheck

clean_pj:
	- @cd pjproject-2.8 && make uninstall && make clean && make distclean

.PHONY:clean
clean: clean clean_pj
