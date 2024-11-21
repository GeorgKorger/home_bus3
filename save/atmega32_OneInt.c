#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr_mcu_section.h>

#define F_CPU 1000000
AVR_MCU(F_CPU, "atmega32");
AVR_MCU_VCD_FILE("my_trace_file32.vcd", 1000);
const struct avr_mmcu_vcd_trace_t _mytrace[] _MMCU_ = {
	{ AVR_MCU_VCD_SYMBOL("OUTPUT"), .mask = (1 << 0), .what = (void*)&PORTB, },
};

int main()
{
  OCR0=50;
  // Compare Interrupt erlauben
  TIMSK |= (1<<OCIE0);
  // Global Interrupts aktivieren
  sei();
  // Timer 0 CTC Modus
  TCCR0 = (1<<WGM01);
  // Port B0 Output
  DDRB=1;
  // Timer 0 starten, Prescaler 0, CTC Mode
  PORTB=1;
  //TCNT0=0xFF-128;
  TCCR0 = (1<<CS00)|(1<<WGM01);
  sleep_mode();
  PORTB=0;
 
  cli();
	sleep_mode();
}
/*
Compare Interrupt Handler
*/
ISR (TIMER0_COMP_vect)
{
}
