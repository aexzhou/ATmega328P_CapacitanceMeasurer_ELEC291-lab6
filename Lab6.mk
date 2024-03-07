SHELL=cmd
PORTN=$(shell type COMPORT.inc)
OBJS=Lab6.o usart.o
CPU=-mmcu=atmega328p

Lab6.elf: $(OBJS)
	avr-gcc $(CPU) $(OBJS) -Wl,-Map,Lab6.map -Wl,-u,vfprintf -lprintf_flt -lm -o Lab6.elf
	avr-objcopy -j .text -j .data -O ihex Lab6.elf Lab6.hex
	@echo done!
	
Lab6.o: Lab6.c
	avr-gcc -g -Os -mmcu=atmega328 -c Lab6.c

usart.o: usart.c usart.h
	avr-gcc -g -Os -Wall -mmcu=atmega328p -c usart.c
	
clean:
	@del *.hex *.elf *.o 2>nul

FlashLoad:
	@taskkill /f /im putty.exe /t /fi "status eq running" > NUL
	spi_atmega -CRYSTAL -p -v Lab6.hex
	cmd /c start putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N

putty:
	@taskkill /f /im putty.exe /t /fi "status eq running" > NUL
	cmd /c start putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N

dummy: Lab6.hex Lab6.map
	@echo Hello from dummy!
	