/********************************
* SloJak                    	*
* MIT License               	*
* Copyright 2015 - Mike Szczys  *
* http://jumptuck.com 	    	*
*				                *
********************************/

#define F_CPU 8000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "menu.h"
#include "oledControl.h"
#include "game.h"


/************** Setup a rotary encoder ********************/
/* Atmega168 */
/* encoder port */
#define ENC_CTL	DDRB	//encoder port control
#define ENC_WR	PORTB	//encoder port write
#define ENC_RD	PINB	//encoder port read
#define ENC_A 6
#define ENC_B 7

volatile int8_t knobChange = 0;

/************** Setup input buttons *********************/
#define BUT_DDR     DDRD
#define BUT_PORT    PORTD
#define BUT_PIN     PIND
#define BUT_LEFT    (1<<PD5)
#define BUT_SEL     (1<<PD6)
#define BUT_MSK     (BUT_LEFT | BUT_SEL)

//Debounce
unsigned char debounce_cnt = 0;
volatile unsigned char key_press;
unsigned char key_state;

uint8_t goLeft = 0;
uint8_t goSel = 0;

uint8_t message[140] = "HELLOWORLDHELLOWORLDHELLOWORLDHELLOWORLDHELLOWORLDHELLOWORLDHELLOWORLD\0";

/**************** Prototypes *************************************/
unsigned char get_key_press(unsigned char key_mask);
void init_IO(void);
void init_interrupts(void);
int main(void);
/**************** End Prototypes *********************************/

/*********************
* Debounce Functions *
*********************/

unsigned char get_key_press(unsigned char key_mask)
{
  cli();               // read and clear atomic !
  key_mask &= key_press;                        // read key(s)
  key_press ^= key_mask;                        // clear key(s)
  sei();
  return key_mask;
}

void init_IO(void){
    //Rotary Encoder
    ENC_CTL &= ~(1<<ENC_A | 1<<ENC_B);
    ENC_WR |= 1<<ENC_A | 1<<ENC_B;

    //Buttons
    BUT_DDR &= ~(BUT_LEFT | BUT_SEL);      //Set as input
    BUT_PORT |= BUT_LEFT | BUT_SEL;      //Enable pull-up
}

void init_interrupts(void) {
    //Pin change interrupts for the rotary encoder
    PCICR |= 1<<PCIE0;      //enable PCINT0_vect  (PCINT0..7 pins)
    PCMSK0 |= 1<<PCINT6;    //interrupt on PCINT6 pin
    PCMSK0 |= 1<<PCINT7;    //interrupt on PCINT7 pin

    //Timer overflow for debounce and systick
    TCCR0B |= 1<<CS02 | 1<<CS00;		//Divide by 1024
    TIMSK0 |= 1<<TOIE0;			//enable timer overflow interrupt

    sei();
}

int main(void)
{
    init_IO();
    init_interrupts();
    oledInit();
    _delay_ms(200);

    oledSetCursor(cursX, cursY);
    putChar(66,1);
    advanceCursor(6);

    compose();

    initMenu();

    while(1)
    {
        if (get_key_press(BUT_SEL))
        {
            //Lookup and execute action
            doSelect[optionIndex]();
        }

        if (get_key_press(BUT_LEFT))
        {
            doBack();
        }

        if (knobChange) {
            if (knobChange > 0) {
                knobLeft();
            }
            else {
                knobRight();
            }
            knobChange = 0;
        }
        
        if (game_running && move_tick) {
            serviceGame();
            move_tick = 0;
        }
    }
}

ISR(PCINT0_vect) {
    /* Encoder service code is from Circuits@Home */
    /* https://www.circuitsathome.com/mcu/rotary-encoder-interrupt-service-routine-for-avr-micros */

    static uint8_t old_AB = 3;  //lookup table index
    static int8_t encval = 0;   //encoder value
    static const int8_t enc_states [] PROGMEM = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};  //encoder lookup table

    old_AB <<=2;  //remember previous state
    old_AB |= ((ENC_RD>>6) & 0x03 ); //Shift magic to get PB6 and PB7 to LSB
    encval += pgm_read_byte(&(enc_states[( old_AB & 0x0f )]));
    /* post "Navigation forward/reverse" event */
    if( encval < -3 ) {  //four steps forward
        knobChange = -1;
        encval = 0;
    }
    else if( encval > 3  ) {  //four steps backwards
        knobChange = 1;
        encval = 0;
    }
}

ISR(TIMER0_OVF_vect) {
    static unsigned char ct0, ct1;
    unsigned char i;

    TCNT0 = (unsigned char)(signed short)-(F_CPU / 1024 * 10e-3 + 0.5);   // preload for 10ms

    i = key_state ^ ~BUT_PIN;    // key changed ?
    ct0 = ~( ct0 & i );          // reset or count ct0
    ct1 = ct0 ^ (ct1 & i);       // reset or count ct1
    i &= ct0 & ct1;              // count until roll over ?
    key_state ^= i;              // then toggle debounced state
    key_press |= key_state & i;  // 0->1: key press detect
    
    if (++timingDelay >= 20) {
        ++move_tick;
    }
}
