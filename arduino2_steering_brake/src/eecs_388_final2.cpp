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

/*BMP180 constants (BMP180 datasheet sections 3.4, 4, 5.2, 5.4, 5.5)*/
#define BMP180_ADDR     (0x77)
#define BMP180_WRITE    ((BMP180_ADDR << 1) | 0)   /* 0xEE */
#define BMP180_READ     ((BMP180_ADDR << 1) | 1)   /* 0xEF */
#define BASE_CAL_ADDR   (0xAA)
#define ID_REG_ADDR     (0xD0)
#define BMP180_CHIP_ID  (0x55)
#define CTRL_MEAS_ADDR  (0xF4)
#define ADC_MSB_ADDR    (0xF6)
#define CTRL_TEMP_CMND  (0x2E)

/*Engine temperature thresholds, tenths of a degree F*/
#define TEMP_COLD_F10   (700)     /* below 70.0 F -> Blue  */
#define TEMP_HOT_F10    (800)     /* above 80.0 F -> Red   */

/*Engine temperature LEDs (second, separate LED circuit; pins unused elsewhere)*/
#define TEMP_LED_BLUE   GPIO_8
#define TEMP_LED_GREEN  GPIO_9
#define TEMP_LED_RED    GPIO_10

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
 *   BMP180 Engine Temperature (Task 5)
 *******************************************************************************/
typedef struct {
    int16_t  cal_AC1;
    int16_t  cal_AC2;
    int16_t  cal_AC3;
    uint16_t cal_AC4;
    uint16_t cal_AC5;
    uint16_t cal_AC6;
    int16_t  cal_B1;
    int16_t  cal_B2;
    int16_t  cal_MB;
    int16_t  cal_MC;
    int16_t  cal_MD;
} bmp180_calibration;

bmp180_calibration calib_data;
uint8_t bmp180_ok = 0;
int32_t engine_temp_f10 = 0;   // last reading, tenths of a degree F

static uint8_t read_register(uint8_t reg_addr)
{
    i2c_start();
    i2c_write_address(BMP180_WRITE);
    i2c_write(reg_addr);
    i2c_start();
    i2c_write_address(BMP180_READ);
    uint8_t value = i2c_read_nack();
    i2c_stop();
    return value;
}

static void read_calibration(bmp180_calibration *calib)
{
    uint16_t words[11];
    i2c_start();
    i2c_write_address(BMP180_WRITE);
    i2c_write(BASE_CAL_ADDR);
    i2c_start();
    i2c_write_address(BMP180_READ);

    for (uint8_t i = 0; i < 11; i++) {
        uint8_t msb = i2c_read_ack();
        uint8_t lsb = (i == 10) ? i2c_read_nack() : i2c_read_ack();
        words[i] = ((uint16_t)msb << 8) | lsb;
    }
    i2c_stop();

    calib->cal_AC1 = (int16_t)words[0];
    calib->cal_AC2 = (int16_t)words[1];
    calib->cal_AC3 = (int16_t)words[2];
    calib->cal_AC4 = words[3];
    calib->cal_AC5 = words[4];
    calib->cal_AC6 = words[5];
    calib->cal_B1  = (int16_t)words[6];
    calib->cal_B2  = (int16_t)words[7];
    calib->cal_MB  = (int16_t)words[8];
    calib->cal_MC  = (int16_t)words[9];
    calib->cal_MD  = (int16_t)words[10];
}

static uint16_t read_temperature_adc()
{
    // Start temperature conversion.
    i2c_start();
    i2c_write_address(BMP180_WRITE);
    i2c_write(CTRL_MEAS_ADDR);
    i2c_write(CTRL_TEMP_CMND);
    i2c_stop();
    delay_ms(5);  // At least 4.5 ms

    // Read UT from 0xF6 (MSB), 0xF7 (LSB).
    i2c_start();
    i2c_write_address(BMP180_WRITE);
    i2c_write(ADC_MSB_ADDR);
    i2c_start();
    i2c_write_address(BMP180_READ);
    uint8_t msb = i2c_read_ack();
    uint8_t lsb = i2c_read_nack();
    i2c_stop();

    return ((uint16_t)msb << 8) | lsb;
}

/* Returns 1 on success and writes temperature in tenths of a degree F. */
static uint8_t get_temperature_f10(int32_t *temp_f10)
{
    uint16_t ut = read_temperature_adc();

    int32_t x1 = (((int32_t)ut - (int32_t)calib_data.cal_AC6)
                  * (int32_t)calib_data.cal_AC5) >> 15;
    if (x1 + (int32_t)calib_data.cal_MD == 0) {
        return 0;
    }
    int32_t x2 = ((int32_t)calib_data.cal_MC * 2048L)
                 / (x1 + (int32_t)calib_data.cal_MD);
    int32_t b5 = x1 + x2;
    int32_t temp_c10 = (b5 + 8) >> 4;          // tenths of a degree C

    *temp_f10 = ((temp_c10 * 9) / 5) + 320;    // tenths of a degree F
    return 1;
}

static void engine_temp_leds(int32_t temp_f10)
{
    if (temp_f10 < TEMP_COLD_F10) {
        // Cold engine, drive gently
        gpio_write(TEMP_LED_BLUE, ON);
        gpio_write(TEMP_LED_GREEN, OFF);
        gpio_write(TEMP_LED_RED, OFF);
    } else if (temp_f10 <= TEMP_HOT_F10) {
        // Normal condition
        gpio_write(TEMP_LED_BLUE, OFF);
        gpio_write(TEMP_LED_GREEN, ON);
        gpio_write(TEMP_LED_RED, OFF);
    } else {
        // Engine overheat
        gpio_write(TEMP_LED_BLUE, OFF);
        gpio_write(TEMP_LED_GREEN, OFF);
        gpio_write(TEMP_LED_RED, ON);
    }
}

/* Read the sensor and update the engine LEDs. Called from loop(), never an ISR. */
static void update_engine_temp()
{
    if (!bmp180_ok) {
        return;
    }

    int32_t temp_f10;
    if (!get_temperature_f10(&temp_f10)) {
        ser_printf("Invalid temperature data or calibration.");
        return;
    }

    engine_temp_f10 = temp_f10;
    engine_temp_leds(temp_f10);
}

/* Print a tenths value like 72.4 (handles negatives). */
static void print_status()
{
    int32_t mag = (engine_temp_f10 < 0) ? -engine_temp_f10 : engine_temp_f10;
    ser_printf("dist_low=%u state=%u temp=%s%ld.%ld F",
               (unsigned int)dist_low, (unsigned int)current_state,
               (engine_temp_f10 < 0) ? "-" : "",
               (long)(mag / 10), (long)(mag % 10));
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

//Setup engine temperature LEDs
gpio_mode(TEMP_LED_BLUE, GPIO_OUTPUT);
gpio_mode(TEMP_LED_GREEN, GPIO_OUTPUT);
gpio_mode(TEMP_LED_RED, GPIO_OUTPUT);

//Setup GPIO_6 for PWM output
gpio_mode(GPIO_6, GPIO_OUTPUT);

//Setup BMP180: init I2C, verify chip ID, read calibration once
i2c_init();
delay_ms(10);    // Allow sensor startup
uint8_t chip_id = read_register(ID_REG_ADDR);
if (chip_id == BMP180_CHIP_ID) {
    read_calibration(&calib_data);
    bmp180_ok = 1;
    ser_printf("BMP180 ready");
    update_engine_temp();
} else {
    ser_printf("BMP180 not found (ID 0x%02X). Check wiring.", (unsigned int)chip_id);
}

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
      // Between angles (~every 500 ms): read engine temp, update its LEDs.
      update_engine_temp();
      // DEBUG: Arduino 2's TX goes to USB, so this shows in the serial monitor (115200)
      print_status();
      for(int x = 0; x < 25; ++x){
        steering(angle_values[i]);
      }
    }
  }
}
