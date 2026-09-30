#include "gpio_stm32.h"

/* 正点原子 STM32F429 阿波罗：LED0 = PF9，低电平点亮 */
#define LED0_PIN        ((uint8_t)9U)
#define DELAY_LOOPS     (200000UL)

static volatile int gpio_check_result;

static void delay_loops(uint32_t loops)
{
	volatile uint32_t i;

	for (i = 0U; i < loops; i++) {
		/* busy wait */
	}
}

int main(void)
{
	int err;
	int level;

	gpio_check_result = -1;
	gpio_stm32_init_ports();

	err = gpio_pin_configure(&gpiof, LED0_PIN, GPIO_OUTPUT_INACTIVE | GPIO_ACTIVE_LOW);
	if (err == 0) {
		err = gpio_pin_set(&gpiof, LED0_PIN, 1);
	}
	if (err == 0) {
		level = gpio_pin_get(&gpiof, LED0_PIN);
		if (level == 1) {
			gpio_check_result = 0;
		} else {
			gpio_check_result = level;
		}
	} else {
		gpio_check_result = err;
	}

	while (1) {
		if (gpio_check_result == 0) {
			(void)gpio_pin_toggle(&gpiof, LED0_PIN);
		}
		delay_loops(DELAY_LOOPS);
	}
}
