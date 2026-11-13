/******************************************************************************
 *   Header Includes
 *******************************************************************************/
#include <Arduino.h>
#include <stdint.h>
#include <stdio.h>

#include "eecs_388_lib.h"

/******************************************************************************
 *   Constant definitions
 *******************************************************************************/

/*ServoMotor constants*/
#define SERVO_PULSE_MAX (2400)    /* 2400 us */
#define SERVO_PULSE_MIN (544)     /* 544 us */
#define SERVO_PERIOD    (20000)   /* 20000 us (20ms) */
#define MIN_ANGLE       (0)       /* degrees */
#define MAX_ANGLE       (180)     /* degrees */

volatile uint16_t servo_pulse_ticks = 23;
volatile uint8_t servo_output_high = 0;
const uint8_t angle_values[] = {10, 25, 75, 45, 100, 40, 125, 15, 150, 50, 170};

/******************************************************************************
 *   Receive from Arduino 1
 *******************************************************************************/
volatile uint8_t dist_low = 0;
 ISR(USART_RX_vect) {
  dist_low = UDR0;
 }

/******************************************************************************
 *   Local function prototypes
 *******************************************************************************/
static void steering(int pos);


void timer1_init()
{
    // Stop Timer1
    TCCR1A = 0;
    TCCR1B = 0;

    // CTC mode
    TCCR1B |= (1 << WGM12);

    // Set compare match value
    OCR1A = 1561;

    // Enable compare match interrupt
    TIMSK1 |= (1 << OCIE1A);

    // Prescaler = 1024
    TCCR1B |= (1 << CS12) | (1 << CS10);
}


// ISR FOR LED
#define SAFE_STATE       0
#define BRAKE_LIGHTLY    1
#define BRAKE_HARD       2
#define UNSAFE_STATE     3

/* LED pins: change these two lines if your colors come out swapped.
   D13 = PB5, D12 = PB4 */
#define RED_BIT    PB5   /* GPIO_13 */
#define GREEN_BIT  PB4   /* GPIO_12 */

volatile uint8_t current_state = SAFE_STATE;

ISR(TIMER1_COMPA_vect)
{

    // Determine braking state based on dist_low


    
    if (dist_low > 200){
    
        current_state = SAFE_STATE;
    }
    else if (dist_low > 100)
    {
        current_state = BRAKE_LIGHTLY;
    }
    else if (dist_low > 60)
    {
        current_state = BRAKE_HARD;
    }
    else
    {
        current_state = UNSAFE_STATE;
    }

    // Update LEDs based on state

    switch (current_state)
    {
        case SAFE_STATE:
            // Green ON, Red OFF
            PORTB |=  (1 << GREEN_BIT);
            PORTB &= ~(1 << RED_BIT);
            break;

        case BRAKE_LIGHTLY:
            // Green ON, Red ON (Yellow)
            PORTB |= (1 << GREEN_BIT);
            PORTB |= (1 << RED_BIT);
            break;

        case BRAKE_HARD:
            // Red ON, Green OFF
            PORTB &= ~(1 << GREEN_BIT);
            PORTB |=  (1 << RED_BIT);
            break;

        case UNSAFE_STATE:
            // Flash red every 100ms
            PORTB &= ~(1 << GREEN_BIT);
            PORTB ^=  (1 << RED_BIT);
            break;
    }
  }



/******************************************************************************
 *   Function: steering() - Steering
 *      Pre condition: 
 *          None
 *      Post condition: 
 *          Control the servomotor with GPIO_6
 *******************************************************************************/
static void steering(int pos)
{
    if (pos < 0)
        pos = 0;

    if (pos > 180)
        pos = 180;

    uint16_t pulse_us =
        SERVO_PULSE_MIN +
        ((uint32_t)pos * (SERVO_PULSE_MAX - SERVO_PULSE_MIN)) / 180;

    gpio_write(GPIO_6, ON);
    delay_us(pulse_us);
    gpio_write(GPIO_6, OFF);
    delay_us(10000 - pulse_us);
    delay_us(10000);

}

/******************************************************************************
 *   Function: setup() - Initializes the Arduino System
 *      Pre condition: 
 *          Hardware must be properly connected (BMP180 sensors, etc.)
 *      Post condition: 
 *          Runs initialization calls one time on power up
 *          UART is initialized for ser_printf()
 *          I2C is initialized for communication with BMP180
 *          calib_data is filled with calibration data from BMP180
 *******************************************************************************/
void setup() 
{
uart_init();     // Initialize UART for serial output
sei();
timer1_init();
//Setup Auto-break LEDs for dist_low
gpio_mode(GPIO_13, GPIO_OUTPUT); //RED
gpio_mode(GPIO_12, GPIO_OUTPUT); //GREEN
gpio_mode(GPIO_11, GPIO_OUTPUT); //BLUE
UCSR0B |= (1 << RXCIE0);

//Setup GPIO_6 for PWM output
gpio_mode(GPIO_6, GPIO_OUTPUT);

ser_printf("System Initialized");
steering(90);
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
  while(1){
    for(int i = 0; i < 11; ++i){
      // DEBUG: Arduino 2's TX goes to USB, so this shows in the serial monitor (115200)
      ser_printf("dist_low=%u state=%u\n", dist_low, current_state);
      for(int x = 0; x < 25; ++x){
        steering(angle_values[i]);
      }
    }
  }
}
