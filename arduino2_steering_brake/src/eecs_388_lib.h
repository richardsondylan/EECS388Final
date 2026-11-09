#ifndef __EECS388_LIB_H__
#define __EECS388_LIB_H__

#include <stdint.h>

/******************************************************************************
 *   generic definitions
 *******************************************************************************/
#define ON              (0x1)
#define OFF             (0x0)
#define GPIO_OUTPUT     (0x1)
#define GPIO_INPUT      (0x0)

/******************************************************************************
 *   Arduino PIN definitions
 *******************************************************************************/
#define GPIO_13         (13)    /* Built-in LED */
#define GPIO_12         (12)
#define GPIO_11         (11)    /* PWM */
#define GPIO_10         (10)    /* PWM */
#define GPIO_9          (9)     /* PWM */
#define GPIO_8          (8)
#define GPIO_7          (7)
#define GPIO_6          (6)     /* PWM */
#define GPIO_5          (5)     /* PWM */
#define GPIO_4          (4)
#define GPIO_3          (3)     /* PWM */
#define GPIO_2          (2)
#define GPIO_1          (1)     /* UART0_TX */
#define GPIO_0          (0)     /* UART0_RX */

/******************************************************************************
 *   eecs388 library APIs (similiar to Arduino)
 *******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

void delay_ms(unsigned long ms);
void delay_us(uint16_t us);
void gpio_mode(uint8_t pin, uint8_t mode);
void gpio_write(uint8_t pin, uint8_t val);
void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
void i2c_write_address(uint8_t address);
void i2c_write(uint8_t data);
uint8_t i2c_read_ack(void);
uint8_t i2c_read_nack(void);
void scan_i2c_bus(void);
void ser_printf(const char *format, ...);
void ser_printline(const char *str);
char ser_read();
void ser_write(const char c);
void uart_init();

#ifdef __cplusplus
}
#endif

#endif // __EECS388_LIB_H__