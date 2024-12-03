#define F_CPU 1000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <stdio.h>

#include <avr_mcu_section.h>

AVR_MCU(1000000, "atmega32");
AVR_MCU_VCD_FILE("puls.vcd", 1000);

const struct avr_mmcu_vcd_trace_t _mytrace[] _MMCU_ = {
	{ AVR_MCU_VCD_SYMBOL("PIND2"), .mask = (1 << 2), .what = (void*)&PIND, },
};

//Variablen für die Counts
volatile unsigned int cnt = 8;
volatile unsigned int cnt_int = 0;

static int uart_putchar(char c, FILE *stream) {
  if (c == '\n')
    uart_putchar('\r', stream);
  loop_until_bit_is_set(UCSRA, UDRE);
  UDR = c;
  return 0;
}
static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL,
                                         _FDEV_SETUP_WRITE);

int main()
{
  stdout = &mystdout;
  uint8_t avrIdx;
  _EEGET(avrIdx,0);
  printf("%d: Hallo\n",avrIdx);
  PORTD=0;
  DDRD=0;
  OCR0=64-1; //sollte 64*8 = 512us
  // Compare Interrupt erlauben
  TIMSK |= (1<<OCIE0);
  
  //MCUCR |= (1<<ISC01); //Ext INT0 trigger on falling edge
  MCUCR |= (1<<ISC00); //Ext INT0 trigger on every edge
  GICR  |= (1<<INT0);  //Ext INT0 enable

  // Global Interrupts aktivieren
  sei();

  // Timer 0 starten CTC Modus Prescaler 8
  TCCR0 = (1<<WGM01)|(1<<CS01);

  while(cnt--)
  {
    sleep_mode();
    DDRD=1<<2;
    printf("%d: PIND: %d\n",avrIdx, PIND);
    sleep_mode();
    DDRD=0;
    printf("%d: PIND: %d\n",avrIdx, PIND);
  }

  printf("%d: cnt_int: %d\n",avrIdx, cnt_int);

  cli();
	sleep_mode();
}

/*
Compare Interrupt Handler
*/
ISR (TIMER0_COMP_vect)
{
}ISR (INT0_vect)
{
  cnt_int++;
}
