
PSP_EBOOT_TITLE 		= ISOMETRIC
TARGET 					= isometric

OBJS 					= sha/sha256.o \
						  error/error.o \
						  umd/umd.o \
						  ciso/ciso.o \
						  main.o
						  
BUILD_PRX				= 1
EXTRA_TARGETS 			= EBOOT.PBP
LIBS 					= -lpspumd -lz
CFLAGS 					= -O2 -G0 -Wall
CXXFLAGS 				= $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS 				= $(CFLAGS)
PSP_EBOOT_ICON 			= ICON0.PNG
PSPSDK					= $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

#WINDOWS COPY RULES
#example: make clean && make && make copy_e
copy_d:
	rm -rf D:\PSP\GAME\$(PSP_EBOOT_TITLE)
	mkdir D:\PSP\GAME\$(PSP_EBOOT_TITLE)
	cp -Rp EBOOT.PBP D:\PSP\GAME\$(PSP_EBOOT_TITLE)\EBOOT.PBP
	
copy_e:
	rm -rf E:\PSP\GAME\$(PSP_EBOOT_TITLE)
	mkdir E:\PSP\GAME\$(PSP_EBOOT_TITLE)
	cp -Rp EBOOT.PBP E:\PSP\GAME\$(PSP_EBOOT_TITLE)\EBOOT.PBP

copy_f:
	rm -rf E:\PSP\GAME\$(PSP_EBOOT_TITLE)
	mkdir E:\PSP\GAME\$(PSP_EBOOT_TITLE)
	cp -Rp EBOOT.PBP E:\PSP\GAME\$(PSP_EBOOT_TITLE)\EBOOT.PBP
