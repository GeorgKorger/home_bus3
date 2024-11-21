#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr_mcu_section.h>

AVR_MCU(1000000, "atmega32");
AVR_MCU_VCD_FILE("my_trace_file.vcd", 1000);

const struct avr_mmcu_vcd_trace_t _mytrace[] _MMCU_ = {
	{ AVR_MCU_VCD_SYMBOL("OUTPUT"), .mask = (1 << 0), .what = (void*)&PORTB, },
};

//Variablen für die Counts
volatile unsigned int cnt;

int main()
{
  DDRB=1;
  OCR0=64-1; //sollte 64*8 = 512us ergeben  
  // Compare Interrupt erlauben
  TIMSK |= (1<<OCIE0);

  // Global Interrupts aktivieren
  sei();

  // Timer 0 starten CTC Modus Prescaler 8
  TCCR0 = (1<<WGM01)|(1<<CS01);
  //TCCR0 = 10;
  //TCCR0 = 1; //kein Prescaler, Normal Mode -> 256us
  //TCCR0 = 2; //Prescaler 8, Normal Mode -> 2048us
  while(1)
  {
    sleep_mode();
    PORTB=1;
    sleep_mode();
    PORTB=0;
  }

  cli();
	sleep_mode();
}

/*
Compare Interrupt Handler
*/
ISR (TIMER0_COMP_vect)
{
}
