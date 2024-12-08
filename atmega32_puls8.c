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

//Variablen für die Interrupts
volatile unsigned int cnt_int = 0;
volatile unsigned int cnt_INT0;

static int uart_putchar(char c, FILE *stream) {
  if (c == '\n')
    uart_putchar('\r', stream);
  loop_until_bit_is_set(UCSRA, UDRE);
  UDR = c;
  return 0;
}
static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL,
                                         _FDEV_SETUP_WRITE);

void my_sleep(void) {
  uint8_t o = cnt_int;
  while(cnt_int == o);
}

void send_pulses(uint8_t ai) {
  uint8_t cnt = 4;
  TCNT0 = 0; //Zurücksetzen
  OCR0=64-1; //64*8 = 512us
  // Compare Interrupt erlauben
  TIMSK |= (1<<OCIE0);
  // Timer 0 starten CTC Modus Prescaler 8
  TCCR0 = (1<<WGM01)|(1<<CS01);

  my_sleep(); // ein extra Startbit
  my_sleep(); // ein extra Startbit
  while(cnt--)
  {
    my_sleep();
    DDRD=1<<2;
    //printf("%d: PIND: %d\n",ai, PIND);
    my_sleep();
    DDRD=0;
    //printf("%d: PIND: %d\n",ai, PIND);
  }
  //Timer 0 stoppen
  TCCR0 = 0;
}

void wait_for_pulses(uint8_t ai) {
  cnt_INT0 = 0;
  MCUCR |= (1<<ISC01); //Ext INT0 trigger on falling edge
  //MCUCR |= (1<<ISC00); //Ext INT0 trigger on every edge
  GICR  |= (1<<INT0);  //Ext INT0 enable
  while(cnt_INT0<4) {
    my_sleep();
    //printf("%d: cnt_INT0: %d\n",ai, cnt_INT0);
  }
  GICR |= 0; //Ext INT0 disable
}

int main()
{
  stdout = &mystdout;
  uint8_t avrIdx;
  _EEGET(avrIdx,0);
  printf("%d: Hallo\n",avrIdx);

  //Port D vorbereiten
  PORTD=0;
  DDRD=0;
  
  sei();    // Global Interrupts aktivieren
  
  if (avrIdx == 0) {
    send_pulses(avrIdx);
    wait_for_pulses(avrIdx);
  }
  if (avrIdx == 1) {
    wait_for_pulses(avrIdx);
    send_pulses(avrIdx);
  }
  cli();
  printf("%d: cnt_INT0: %d\n",avrIdx, cnt_INT0);

	sleep_mode();
}

/*
Compare Interrupt Handler
*/
ISR (TIMER0_COMP_vect)
{
  cnt_int++;
}

ISR (INT0_vect)
{
  cnt_int++;
  cnt_INT0++;
}
