/******************************************************************************
 *   Header Includes
 *******************************************************************************/
#include <Arduino.h>
#include <stdint.h>

#include "eecs_388_lib.h"

/******************************************************************************
 *   Function: setup() - Initializes the Arduino System
 *      Pre condition:
 *          Hardware must be properly connected (sensors, LEDS, etc.)
 *      Post condition:
 *          Runs initialization calls one time on power up
 *          Input/Output (IO) pins are configured
 *******************************************************************************/
void setup()
{
    // Enable global interrupts.
    sei();
    uart_init();

    // Enable receiver interrupt.
    UCSR0B |= (1 << RXCIE0);
return;
}

void transmit_data_low(){
    // Enable data empty interrupt flag.
    UCSR0B |= (1 << UDRIE0);
}

ISR(USART_UDRE_vect){
    // Only need to send low byte for distance conditions.
    // However, when high byte is greater than 0, we must
    // set dist_low to 255.
    //
    // Also, Atmega328p has RX interrupt at higher priority than UDRE,
    // so the data will always be available for TX.
    //
    if(dist_high > 0){
        UDR0 = 255;
    } else {
        UDR0 = dist_low;
    }

    // Disable interrupt to stop sending.
    UCSR0B &= ~(1 << UDRIE0);
}


// ISR Implementation of uart_read to reduce possible latency
// See PG #49 of the ATmega328P datasheet for interrupt vectors.
volatile uint8_t dist_low = 0;
volatile uint8_t dist_high = 0;
uint8_t current_byte_received = 0;
ISR(USART_RX_vect){
    uint8_t udrn_read = UDR0;
    if(current_byte_received == 2){
        dist_low = udrn_read;
    }
    else if(current_byte_received == 3){
        dist_high = udrn_read;
    }

    current_byte_received++;

    if(current_byte_received > 8){
        current_byte_received = 0;
    }

    // Enable transmit
    transmit_data_low();
}

/******************************************************************************
 *   Function: loop() - Main execution loop
 *      Pre condition:
 *          setup() has been executed and system is initialized
 *      Post condition:
 *          Performs a single iteration of the system's function
 *          Repeates indefinetely unless the board is reset or powered off
 *******************************************************************************/



void loop()
{

ser_printline("Setup completed.");
ser_write( '\n' );
while( 1 )
    {
    }

return;
}
