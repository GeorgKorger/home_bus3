#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr_mcu_section.h>

AVR_MCU(1000000, "atmega32");

int main()
{
  cli();
	sleep_mode();
}

