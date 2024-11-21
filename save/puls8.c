#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include <signal.h>

#include "sim_avr.h"
#include "sim_elf.h"
#include "sim_core.h"
#include "sim_gdb.h"
#include "sim_hex.h"
#include "sim_vcd_file.h"

//#include "sim_core_declare.h"

avr_t * avr = NULL;

int main(int argc, char *argv[])
{
	elf_firmware_t f;
	const char * fname =  "atmega32_puls8.axf";

	printf("Firmware pathname is %s\n", fname);
	elf_read_firmware(fname, &f);

	printf("firmware %s f=%d mmcu=%s\n", fname, (int)f.frequency, f.mmcu);

	avr = avr_make_mcu_by_name(f.mmcu);
	if (!avr) {
		fprintf(stderr, "%s: AVR '%s' not known\n", argv[0], f.mmcu);
		exit(1);
	}
	avr_init(avr);
	avr_load_firmware(avr, &f);

	// Even if not setup at startup, activate gdb if crashing
	avr->gdb_port = 1234;
	if (0) {
		avr->state = cpu_Stopped;
		avr_gdb_init(avr);
	}

	printf( "\nPULS8 launching:\n");

	int state = cpu_Running;
	while ((state != cpu_Done) && (state != cpu_Crashed))
		state = avr_run(avr);
}
