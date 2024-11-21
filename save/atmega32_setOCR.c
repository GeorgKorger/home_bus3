#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr_mcu_section.h>

AVR_MCU(1000000, "atmega32");

int main()
{
  OCR0=64-1;
  cli();
	sleep_mode();
}

