GPPPARAMS = -m32 -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore
ASPARAMS = --32
LDPARAMS = -melf_i386

objects = loader.o gdt.o kernel.o

%.o: %.cpp
	g++ $(GPPPARAMS) -c $< -o $@

.o: %.cpp
	g++ -c gdt.cpp -o  gdt.o

%.o: %.s
	as $(ASPARAMS) -o $@ $<

mykernel.bin: linker.ld $(objects)
	ld $(LDPARAMS) -T $< -o $@ $(objects)

install: mykernel.bin
	qemu-system-i386 -kernel mykernel.bin -display gtk

debug: mykernel.bin
	qemu-system-i386 -kernel mykernel.bin -display gtk -s -S

clean:
	rm -f *.o mykernel.bin
