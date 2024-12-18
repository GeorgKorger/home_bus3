#define F_CPU 1000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <avr_mcu_section.h>

AVR_MCU(1000000, "atmega32");
AVR_MCU_VCD_FILE("intcnt1_outp.vcd", 1000);

const struct avr_mmcu_vcd_trace_t _mytrace[] _MMCU_ = {
	{ AVR_MCU_VCD_SYMBOL("OUTPUT"), .mask = (1 << 0), .what = (void*)&PORTB, },
};

//Variablen für die Counts
unsigned int cnt=8;

int main()
{
  PORTB=1;
  OCR0=64-1; //sollte 64*8 = 512us

  PORTD=1<<INT0;  //INT0 Pin pulled high
  MCUCR=1<<ISC11; //The falling edge of INT0 generates an interrupt req
  GICR= 1<<INT0;  //External Interrupt Request 0 Enable

  sei();  // Global Interrupts aktivieren

  while(cnt--)
  {
    sleep_mode();
  }
  GICR &= ~(1<<INT0);  //External Interrupt Request 0 Disable
  
  TIMSK |= (1<<OCIE0);  // Compare Interrupt erlauben
  cnt=8;
  TCCR0 = (1<<WGM01)|(1<<CS01);  // Timer 0 starten CTC Modus Prescaler 8
  DDRB=1;
  while(cnt--)
  {
    sleep_mode();
    PORTB=0;
    sleep_mode();
    PORTB=1;
  }

  cli();
	sleep_mode();
}

ISR (INT0_vect)
{
}
/*
Compare Interrupt Handler
*/
ISR (TIMER0_COMP_vect)
{
}
