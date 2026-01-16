VERSION    = 1
SUBVERSION = 1

TOOL = arm-none-eabi
CC = $(TOOL)-gcc
CP = $(TOOL)-objcopy

HOME    =../
BT2     =$(HOME)/bt2.exe -sb 115200 -s 115200 -d 
ISP     =$(HOME)/lpc21isp.exe

PFLAGS = -DMASTER
CFLAGS = -w -g -gdwarf-2 -O2 -mcpu=cortex-m0 -mthumb -nostartfiles -fno-delete-null-pointer-checks \
         -DVERSION=$(VERSION) -DSUBVERSION=$(SUBVERSION) $(PFLAGS)
SRCS =	init.c main.c printf.c system.h lpc111x.h 

USB_COM=$(shell ..\detect_USB_COM)
#USB_COM=//./COM26

all:	burn

burn:	coderom.bin
	$(ISP) -control -wipe -bin coderom.bin $(USB_COM) 115200 12000
		
code.elf:	$(SRCS) linker_scriptRAM.ld Makefile 
	$(CC) $(CFLAGS) -Wl,-Tlinker_scriptRAM.ld -Wl,-Map=a.map -o $@ main.c 
	$(TOOL)-size $@

coderom.elf:	$(SRCS) linker_script.ld Makefile 
	$(CC) $(CFLAGS) -Wl,-Tlinker_script.ld -Wl,-Map=a.map -o $@ main.c 
	$(TOOL)-size $@

code.bin:	code.elf
	$(CP) -O binary $< $@

coderom.bin:	coderom.elf
	$(CP) -O binary $< $@

dis:	coderom.elf
	$(TOOL)-objdump -d $< >coderom.lst

clean:
	rm -f *.elf *.lst a.map *~
