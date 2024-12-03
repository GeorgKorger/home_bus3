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
#include "avr_eeprom.h"

static avr_t *avrs[] = {NULL,NULL};
#define AVRS_LEN sizeof(avrs) / sizeof(avr_t*)

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

static void d_out_notify(avr_irq_t *irq, uint32_t value, void *param)
{
  uint8_t avrIdx = *((uint8_t *) param);
  avr_t *avr = avrs[avrIdx];
  avr_ioport_state_t state;
  
  if (avr_ioctl(avr, AVR_IOCTL_IOPORT_GETSTATE('D'), &state) == 0)
  {
    uint8_t oddr  = value;
    uint8_t oport = state.port;
    if ((oddr ^ state.ddr) & (1<<2)) { //betrifft PIND2
        printf("avr%d: DDR Bit for PIND2 changed to %s\n", avrIdx, (value & (1<<2))?"ACTIV":"passiv");
        uint8_t ddr, port, pin = 1<<2;
        for(uint8_t i = 0; i < AVRS_LEN; i++) {
          if(i == avrIdx) {
            ddr  = oddr;
            port = oport;
          }
          else {
            if (avr_ioctl(avrs[i], AVR_IOCTL_IOPORT_GETSTATE('D'), &state) == 0) {
              ddr = state.ddr;
              port = state.port;
            }
            else {
              break;
            }
          }
          if( ddr & ~port & 1<<2 ) { //DDRbit ist 1 und PORTbit ist 0 -> Der Bus wird 0
            pin = 0;
            break;
          }
        }
        for(uint8_t i = 0; i < AVRS_LEN; i++) {
  			  avr_raise_irq(avr_io_getirq(avrs[i], AVR_IOCTL_IOPORT_GETIRQ('D'), 2), pin);
		  }
	  }
	}
}


int
main(
		int argc,
		char *argv[])
{
	printf("len of avrs: %ld\n",AVRS_LEN);
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

	/* Adressen ins eeprom schreiben */
  uint8_t device_address1 = 1;
	avr_eeprom_desc_t d1 = { .ee = &device_address1, .offset = 0, .size = 1 }; 
  avr_ioctl(avrs[0], AVR_IOCTL_EEPROM_SET, &d1);
  uint8_t device_address2 = 2;
	avr_eeprom_desc_t d2 = { .ee = &device_address2, .offset = 0, .size = 1 }; 
  avr_ioctl(avrs[1], AVR_IOCTL_EEPROM_SET, &d2);
	
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
  //avr_irq_register_notify(base_irq + IOPORT_IRQ_REG_PORT, d_out_notify, &avr0);
  avr_irq_register_notify(base_irq + IOPORT_IRQ_DIRECTION_ALL, d_out_notify, &avr0);

  base_irq = avr_io_getirq(avrs[1], ioctl, 0);
  //avr_irq_register_notify(base_irq + IOPORT_IRQ_REG_PORT, d_out_notify, &avr1);
  avr_irq_register_notify(base_irq + IOPORT_IRQ_DIRECTION_ALL, d_out_notify, &avr1);


	if (gdb) {
		avrs[0]->state = cpu_Stopped;
		avr_gdb_init(avrs[0]);
	}


	signal(SIGINT, sig_int);
	signal(SIGTERM, sig_int);

  // pullup PIND2
  avr_raise_irq(avr_io_getirq(avrs[0], AVR_IOCTL_IOPORT_GETIRQ('D'), 2), 1);
  avr_raise_irq(avr_io_getirq(avrs[1], AVR_IOCTL_IOPORT_GETIRQ('D'), 2), 1);

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
