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

#include "sim_core_decl.h"

#include "avr_ioport.h"

static avr_t *avrs[] = {NULL,NULL};
//static avr_t      * avr0 = NULL, * avr1 = NULL;

static void
sig_int(
		int sign)
{
	printf("signal caught, simavr terminating\n");
	if (avrs[0])
		avr_terminate(avrs[0]);
	if (avrs[1])
		avr_terminate(avrs[1]);
	exit(0);
}

static uint8_t pullup_mask = (1<<2)|(1<<0);

static void d_out_notify(avr_irq_t *irq, uint32_t value, void *param)
{
  int avrIdx = *((int*)param);
  avr_t *avr = avrs[avrIdx];
  uint8_t port,ddr,pin,mask;
  avr_ioport_state_t state;
  if (avr_ioctl(avr, AVR_IOCTL_IOPORT_GETSTATE('D'), &state) == 0)
  {
    if (irq->irq == IOPORT_IRQ_DIRECTION_ALL) {
        printf("avr%d: DDR Register changed %02x\n",avrIdx,value);
        ddr = value;
        port = state.port;
    } else {
        printf("avr%d: Port Register changed %02x\n",avrIdx,value);
        ddr = state.ddr;
        port = value;
    }
    //printf("PORTB %02x DDR %02x PIN %02x\n",state.port, state.ddr, state.pin);
    pin = (~ddr)|port;
    mask = (pin ^ state.pin) & pullup_mask;
	  for (uint8_t i = 0; i < 8; i++) {
		  if (mask & (1 << i)) {
			  avr_raise_irq(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), i), pin & (1 << i));
		  }
	  }
	}
}


int
main(
		int argc,
		char *argv[])
{
	elf_firmware_t f1 = {{0}}, f2 = {{0}};
	uint32_t f_cpu = 1000000;
	int gdb = 0;
	//int list_irqs = 0;
	int log = LOG_ERROR;
	int port = 1234;
	char name[] = "atmega32";
	uint32_t loadBase = AVR_SEGMENT_OFFSET_FLASH;
	//int trace_vectors[8] = {0};
	//int trace_vectors_count = 0;
	const char *vcd_input = NULL;
	char firmware[] = "atmega32_puls8.axf";

  strcpy(f1.mmcu, name);
  strcpy(f2.mmcu, name);
	f1.frequency = f_cpu;
	f2.frequency = f_cpu;
  sim_setup_firmware(firmware, loadBase, &f1, argv[0]);
  sim_setup_firmware(firmware, loadBase, &f2, argv[0]);
  
	// Frequency and MCU type were set early so they can be checked when
	// loading a hex file. Set them again because they can also be set
 	// in an ELF firmware file.
  strcpy(f1.mmcu, name);
  strcpy(f2.mmcu, name);
	f1.frequency = f_cpu;
	f2.frequency = f_cpu;

	avrs[0] = avr_make_mcu_by_name(f1.mmcu);
	if (!avrs[0]) {
		fprintf(stderr, "%s: AVR '%s' not known for avr0\n", argv[0], f1.mmcu);
		exit(1);
	}
	avrs[1] = avr_make_mcu_by_name(f2.mmcu);
	if (!avrs[1]) {
		fprintf(stderr, "%s: AVR '%s' not known for avr1\n", argv[0], f2.mmcu);
		exit(1);
	}

	avr_init(avrs[0]);
	avr_init(avrs[1]);

	avrs[0]->log = (log > LOG_TRACE ? LOG_TRACE : log);
	avrs[1]->log = (log > LOG_TRACE ? LOG_TRACE : log);

	avr_load_firmware(avrs[0], &f1);
	if (f1.flashbase) {
		printf("avr0: Attempted to load a bootloader at %04x\n",
			   f1.flashbase);
		avrs[0]->pc = f1.flashbase;
	}

	avr_load_firmware(avrs[1], &f2);
	if (f2.flashbase) {
		printf("avr1: Attempted to load a bootloader at %04x\n",
			   f2.flashbase);
		avrs[1]->pc = f2.flashbase;
	}

	if (vcd_input) {
		static avr_vcd_t input;

		if (avr_vcd_init_input(avrs[0], vcd_input, &input)) {
			fprintf(stderr,
					"%s: Warning: VCD input file %s failed\n",
					argv[0], vcd_input);
			vcd_input = NULL;
		}
	}

	// even if not setup at startup, activate gdb if crashing
	avrs[0]->gdb_port = port;



  /******** SIMULATION **********/
  static int avr0 = 0;
  static int avr1 = 1; 
  uint32_t   ioctl = (uint32_t)AVR_IOCTL_IOPORT_GETIRQ('D');

  avr_irq_t  *base_irq;
  base_irq = avr_io_getirq(avrs[0], ioctl, 0);
  avr_irq_register_notify(base_irq + IOPORT_IRQ_REG_PORT, d_out_notify, &avr0);
  avr_irq_register_notify(base_irq + IOPORT_IRQ_DIRECTION_ALL, d_out_notify, &avr0);

  base_irq = avr_io_getirq(avrs[1], ioctl, 0);
  avr_irq_register_notify(base_irq + IOPORT_IRQ_REG_PORT, d_out_notify, &avr1);
  avr_irq_register_notify(base_irq + IOPORT_IRQ_DIRECTION_ALL, d_out_notify, &avr1);


	if (gdb) {
		avrs[0]->state = cpu_Stopped;
		avr_gdb_init(avrs[0]);
	}


	signal(SIGINT, sig_int);
	signal(SIGTERM, sig_int);

	/* set avr index (inverted!) on pins PORTB0 and PORTB1
  for(uint8_t i = 0;i<2;i++) {
	  avr_raise_irq(avr_io_getirq(avrs[i], AVR_IOCTL_IOPORT_GETIRQ('B'), 0), ~(i & (1 << 0)));
	  avr_raise_irq(avr_io_getirq(avrs[i], AVR_IOCTL_IOPORT_GETIRQ('B'), 1), ~(i & (1 << 1)));
	}
	*/
	avr_raise_irq(avr_io_getirq(avrs[1], AVR_IOCTL_IOPORT_GETIRQ('B'), 0), 1);
	{
	  int avr0_running = 1, avr1_running = 1;
		for (;;) {
			if(avr0_running) {
			  int state = avr_run(avrs[0]);
			  if (state == cpu_Done || state == cpu_Crashed) {
			    printf("avr0 stopped\n");
				  avr0_running = 0;
				}
      }
			if(avr1_running) {
			  int state = avr_run(avrs[1]);
			  if (state == cpu_Done || state == cpu_Crashed) {
			    printf("avr1 stopped\n");
			    avr1_running = 0;
			  }
			}
			if((!avr0_running) && (!avr1_running)) {
			  break;
			}
		}
	}
	avr_terminate(avrs[0]);
	avr_terminate(avrs[1]);
}
