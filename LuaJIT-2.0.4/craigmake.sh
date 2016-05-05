make HOST_CC="gcc -m32" TARGET_CFLAGS="-L../../lib -I../../include -I../../common -mcpu=arm7tdmi-s -mthumb -mlittle-endian -DLUAJIT_TARGET=LUAJIT_ARCH_ARM -DLUAJIT_OS=LUAJIT_OS_OTHER -DLUAJIT_USE_SYSMALLOC" CROSS=arm-none-eabi- TARGET_SYS=Other 

#make HOST_CC="gcc -m32" TARGET_CFLAGS="-mfloat_abi=soft -mcpu=arm7tdmi-s -mthumb -mlittle-endian -DLUAJIT_TARGET=LUAJIT_ARCH_ARM -DLUAJIT_OS=LUAJIT_OS_OTHER -DLUAJIT_USE_SYSMALLOC" CROSS=arm-none-eabi- TARGET_SYS=Other 

