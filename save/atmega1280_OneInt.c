#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr_mcu_section.h>

AVR_MCU(1000000, "atmega1280");
AVR_MCU_VCD_FILE("my_trace_file1280.vcd", 1000);
const struct avr_mmcu_vcd_trace_t _mytrace[] _MMCU_ = {
	{ AVR_MCU_VCD_SYMBOL("OUTPUT"), .mask = (1 << 0), .what = (void*)&PORTB, },
};

int main()
{
  OCR0A=6;
  // Compare Interrupt erlauben
  TIMSK0 |= (1<<OCIE0A);
  // Global Interrupts aktivieren
  sei();
  // Timer 0 CTC Modus
  TCCR0A = (1<<WGM01);
  // Port B0 Output
  DDRB=1;
  // Timer 0 starten, Prescaler 0
  PORTB=1;
  //TCNT0=0xFF-128;
  TCCR0B = (1<<CS00);
  sleep_mode();
  PORTB=0;
 
  cli();
	sleep_mode();
}
/*
Compare Interrupt Handler
*/
ISR (TIMER0_COMPA_vect)
{
}
