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

volatile uint8_t dist_low = 0;
volatile uint8_t dist_high = 0;
uint8_t current_byte_received = 0;
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




volatile uint16_t distance = 0;
volatile uint16_t rx_count = 0;

ISR(USART_RX_vect)
{
    static uint8_t frame[9];
    static uint8_t index = 0;

    uint8_t data = UDR0;
    rx_count++;

    // Look for first header byte
    if (index == 0)
    {
        if (data == 0x59)
        {
            frame[index++] = data;
        }
    }
    // Look for second header byte
    else if (index == 1)
    {
        if (data == 0x59)
        {
            frame[index++] = data;
        }
        else
        {
            index = 0;
        }
    }
    else
    {
        frame[index++] = data;

        // Complete 9-byte frame
        if (index == 9)
        {
            uint8_t checksum = 0;

            for (uint8_t i = 0; i < 8; i++)
            {
                checksum += frame[i];
            }

            if (checksum == frame[8])
            {
                distance = frame[2] | (frame[3] << 8);
            }

            index = 0;
        }
    }
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

uint16_t current_distance;

    uint8_t oldSREG = SREG;
    cli();
    current_distance = distance;
    SREG = oldSREG;
    ser_printf("Distance: %u cm\n", current_distance);

return;
}
