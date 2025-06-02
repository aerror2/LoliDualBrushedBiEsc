CC = sdcc
stctool = stcgal
protocol = stc15
upload_port = /dev/tty.wchusbserial*

all: build

build: LoliDualBrBiEsc.c Makefile showsize.sh
	@mkdir -p build
	$(CC)   --opt-code-speed LoliDualBrBiEsc.c -o build/LoliDualBrBiEscUsorted.ihx
	srec_cat build/LoliDualBrBiEscUsorted.ihx -intel -o build/LoliDualBrBiEsc.hex -intel
	./showsize.sh build/LoliDualBrBiEsc.hex 0x800

upload:
	$(stctool) -P $(protocol) -p $(upload_port) LoliDualBrBiEsc.ihx

clean:
	rm -f *.asm *.lst *.map *.mem *.rel *.rst *.sym *.lk *.hex *.ihx
	rm -rf build