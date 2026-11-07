/******************************************************************************
 *   Header Includes
 *******************************************************************************/
#include <avr/io.h>
#include <stdint.h>

#include "eecs_388_lib.h"
#include "wiring_private.h"
#include "pins_arduino.h"

/******************************************************************************
 *   Static functions
 *******************************************************************************/
static void turnOffPWM(uint8_t timer)
{
	switch (timer)
	{
		#if defined(TCCR1A) && defined(COM1A1)
		case TIMER1A:   cbi(TCCR1A, COM1A1);    break;
		#endif
		#if defined(TCCR1A) && defined(COM1B1)
		case TIMER1B:   cbi(TCCR1A, COM1B1);    break;
		#endif
		#if defined(TCCR1A) && defined(COM1C1)
		case TIMER1C:   cbi(TCCR1A, COM1C1);    break;
		#endif

		#if defined(TCCR2) && defined(COM21)
		case  TIMER2:   cbi(TCCR2, COM21);      break;
		#endif

		#if defined(TCCR0A) && defined(COM0A1)
		case  TIMER0A:  cbi(TCCR0A, COM0A1);    break;
		#endif

		#if defined(TCCR0A) && defined(COM0B1)
		case  TIMER0B:  cbi(TCCR0A, COM0B1);    break;
		#endif
		#if defined(TCCR2A) && defined(COM2A1)
		case  TIMER2A:  cbi(TCCR2A, COM2A1);    break;
		#endif
		#if defined(TCCR2A) && defined(COM2B1)
		case  TIMER2B:  cbi(TCCR2A, COM2B1);    break;
		#endif

		#if defined(TCCR3A) && defined(COM3A1)
		case  TIMER3A:  cbi(TCCR3A, COM3A1);    break;
		#endif
		#if defined(TCCR3A) && defined(COM3B1)
		case  TIMER3B:  cbi(TCCR3A, COM3B1);    break;
		#endif
		#if defined(TCCR3A) && defined(COM3C1)
		case  TIMER3C:  cbi(TCCR3A, COM3C1);    break;
		#endif

		#if defined(TCCR4A) && defined(COM4A1)
		case  TIMER4A:  cbi(TCCR4A, COM4A1);    break;
		#endif
		#if defined(TCCR4A) && defined(COM4B1)
		case  TIMER4B:  cbi(TCCR4A, COM4B1);    break;
		#endif
		#if defined(TCCR4A) && defined(COM4C1)
		case  TIMER4C:  cbi(TCCR4A, COM4C1);    break;
		#endif
		#if defined(TCCR4C) && defined(COM4D1)
		case TIMER4D:	cbi(TCCR4C, COM4D1);	break;
		#endif

		#if defined(TCCR5A)
		case  TIMER5A:  cbi(TCCR5A, COM5A1);    break;
		case  TIMER5B:  cbi(TCCR5A, COM5B1);    break;
		case  TIMER5C:  cbi(TCCR5A, COM5C1);    break;
		#endif
	}
}

/******************************************************************************
 *   Library functions
 *******************************************************************************/

 /******************************************************************************
 *   Function: delay_ms() - Delay Milliseconds
 *      Pre condition:
 *          None
 *      Post condition:
 *          Delay execution of the next instruction by amount specified by
 * 			'ms'. Configured for 16MHz ATMega328P clock
 *******************************************************************************/
 void delay_ms(unsigned long ms)
{
	uint32_t start = micros();

	while (ms > 0) {
		yield();
		while ( ms > 0 && (micros() - start) >= 1000) {
			ms--;
			start += 1000;
		}
	}
}

 /******************************************************************************
 *   Function: gpio_mode() - GPIO Mode
 *      Pre condition:
 *          This should be called during the setup() initialization function
 *      Post condition:
 *          GPIO specified by 'pin' will be configured in the mode specified by
 * 			'mode' (either GPIO_OUTPUT or GPIO_INPUT)
 *******************************************************************************/
void gpio_mode(uint8_t pin, uint8_t mode)
{
uint8_t bit = digitalPinToBitMask(pin);
uint8_t port = digitalPinToPort(pin);
volatile uint8_t *reg, *out;

if (port == NOT_A_PIN) return;

reg = portModeRegister(port);
out = portOutputRegister(port);

if (mode == GPIO_INPUT) {
	uint8_t oldSREG = SREG;
            cli();
	*reg &= ~bit;
	*out &= ~bit;
	SREG = oldSREG;
} else if (mode == INPUT_PULLUP) {
	uint8_t oldSREG = SREG;
            cli();
	*reg &= ~bit;
	*out |= bit;
	SREG = oldSREG;
} else {
	uint8_t oldSREG = SREG;
            cli();
	*reg |= bit;
	SREG = oldSREG;
    }

return;
}

void gpio_write(uint8_t pin, uint8_t val)
{
uint8_t timer = digitalPinToTimer(pin);
uint8_t bit = digitalPinToBitMask(pin);
uint8_t port = digitalPinToPort(pin);
volatile uint8_t *out;

if (port == NOT_A_PIN) return;
// If the pin that support PWM output, we need to turn it off
// before doing a digital write.
if (timer != NOT_ON_TIMER)
	{
	turnOffPWM(timer);
	}

out = portOutputRegister(port);

uint8_t oldSREG = SREG;
cli();

if (val == OFF) {
	*out &= ~bit;
} else {
	*out |= bit;
}

SREG = oldSREG;
return;
}

void uart_init() {
    /*  Task 1.1 - Update baud rate with ubrr value. Initialize ubrr to calculated value */
	uint16_t ubrr = 16;
	UBRR0H = (ubrr >> 8);
    UBRR0L = ubrr;

	/* Task 1.1 - Setup double speed mode
				  modify U2X0 only to set it in UCSR0A for double speed mode */
	UCSR0A |= 1 << 1;

    UCSR0B = (1 << RXEN0) | (1 << TXEN0 );   // Enable transmitter/receiver
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit data, leaves parity/stop/mode to defaults (no parity, 1 stop, async)
}

void ser_printf(const char *format, ...) {
    char buffer[128];

    va_list args;

	va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    const char *p = buffer;
    while (*p) {
        ser_write(*p++);
    }

    ser_write('\n');
    ser_write('\r');
}

void ser_printline(const char *str) {
    while (*str) {
        ser_write(*str++); // Send characters one by one
    }
    ser_write('\n'); // newline character
    ser_write('\r'); // carriage return
}

char ser_read() {
    while (!(UCSR0A & (1 << RXC0))) {
        // Wait until data is received
    }
    return UDR0; // Return received byte from buffer
}

void ser_write(const char c) {
    while (!(UCSR0A & (1 << UDRE0))) {
        // Wait until transmit buffer is empty
    }
    UDR0 = c; // Load data into transmit register
}
