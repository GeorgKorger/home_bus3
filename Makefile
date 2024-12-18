target=	puls8
firm_src = ${wildcard at*${board}.c}
firmware = ${firm_src:.c=.axf}
simavr = ../..

IPATH = .
IPATH += ${simavr}/examples/shared
IPATH += ${simavr}/examples/parts
IPATH += ${simavr}/include
IPATH += ${simavr}/simavr
IPATH += ${simavr}/simavr/sim

VPATH = .
VPATH += ${simavr}/examples/shared
VPATH += ${simavr}/examples/parts


all: obj ${firmware} ${target} 

include ${simavr}/Makefile.common

atmega32_${target}.axf: atmega32_${target}.c

board = ${OBJ}/${target}.elf

${board} : ${OBJ}/${target}.o
${board} : ${OBJ}/i2c_eeprom.o

${target}: ${board}
	@echo $@ done

clean: clean-${OBJ}
	rm -rf *.hex *.a *.axf ${target} *.vcd .*.swo .*.swp .*.swm .*.swn
