SHELL=cmd
CC=c51
COMPORT=$(shell type COMPORT.inc)
COMPORTc=$(shell type COMPORTc.inc)
OBJS=Lab6.o usart.o lcd.o SoftwareUart.o 
OBJSc=Controller.obj
CPU=-mmcu=atmega328p


Lab6.elf: $(OBJS)
	avr-gcc $(CPU) $(OBJS) -Wl,-Map,Lab6.map -Wl,-u,vfprintf -lprintf_flt -lm -o Lab6.elf
	avr-objcopy -j .text -j .data -O ihex Lab6.elf Lab6.hex
	@echo done!

Controller.hex: $(OBJSc)
	$(CC) $(OBJSc)
	@echo Done!
	
Controller.obj: Controller.c
	$(CC) -c Controller.c
	
Lab6.o: Lab6.c
	avr-gcc --param=min-pagesize=0 -g -Os -mmcu=atmega328 -c Lab6.c

usart.o: usart.c usart.h
	avr-gcc --param=min-pagesize=0 -g -Os -Wall -mmcu=atmega328p -c usart.c

lcd.o: lcd.c usart.h LCD.h
	avr-gcc --param=min-pagesize=0 -g -Os -Wall -mmcu=atmega328p -c lcd.c

SoftwareUart.o: SoftwareUart.c SoftwareUart.h
	avr-gcc --param=min-pagesize=0 -g -Os -Wall -mmcu=atmega328p -c SoftwareUart.c

clean:
	@del *.asm *.lkr *.lst *.map *.map *.o 2>nul

Flash_Device:
	@taskkill /f /im putty.exe /t /fi "status eq running" > NUL
	spi_atmega -CRYSTAL -p -v -d1 Lab6.hex
	cmd /c start putty.exe -serial $(COMPORT) -sercfg 115200,8,n,1,N

Flash_Controller:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	EFM8_prog.exe -ft230 -r -d0 Controller.hex
	cmd /c start putty -serial $(COMPORTc) -sercfg 115200,8,n,1,N

Flash_ALL:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500 
	EFM8_prog.exe -ft230 -r -d0 Controller.hex
	move COMPORT.inc COMPORTc.inc
	spi_atmega -CRYSTAL -p -v -d1 Lab6.hex
	cmd /c start putty -serial $(COMPORTc) -sercfg 115200,8,n,1,N
	cmd /c start putty.exe -serial $(COMPORT) -sercfg 115200,8,n,1,N

putty_Device:
	@taskkill /f /im putty.exe /t /fi "status eq running" > NUL
	cmd /c start putty.exe -serial $(COMPORT) -sercfg 115200,8,n,1,N

putty_Controller:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	cmd /c start putty -serial $(COMPORTc) -sercfg 115200,8,n,1,N

putty_ALL:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	cmd /c start putty -serial $(COMPORTc) -sercfg 115200,8,n,1,N
	cmd /c start putty -serial $(COMPORT) -sercfg 115200,8,n,1,N

dummy: Lab6.hex Lab6.map Controller.hex Controller.map
	@echo Hello from dummy!

explorer:
	cmd /c start explorer .