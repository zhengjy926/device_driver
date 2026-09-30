/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (C) 2025 Savoir-faire Linux, Inc.
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Trimmed from Zephyr v4.4.2 gpio_stm32 / gpioport_mgr / gpio_intc.
 * Target: STM32F429. No Devicetree, PM, HSEM, or pinctrl.
 */

#include "gpio_stm32.h"
#include "gpio_utils.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef STM32F429xx
#error "gpio_stm32.c currently requires STM32F429xx"
#endif

#include "stm32f4xx.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_exti.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_system.h"

#define GPIO_STM32_PINS_PER_PORT    (16U)
#define GPIO_STM32_PIN_MASK         (0xFFFFU)

struct gpio_stm32_config {
	struct gpio_driver_config common;
	GPIO_TypeDef *regs;
	uint32_t port;
	uint32_t clock_enr;
};

struct gpio_stm32_data {
	struct gpio_driver_data common;
	struct gpio_callback *cb;
};

struct gpio_stm32_exti_slot {
	const struct device *dev;
};

static struct gpio_stm32_exti_slot gpio_stm32_exti_owner[GPIO_STM32_PINS_PER_PORT];

static const IRQn_Type gpio_stm32_exti_irqn[GPIO_STM32_PINS_PER_PORT] = {
	EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn, EXTI4_IRQn,
	EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn,
	EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
	EXTI15_10_IRQn, EXTI15_10_IRQn
};

static const struct gpio_stm32_config *gpio_stm32_get_cfg(const struct device *dev)
{
	const struct gpio_stm32_config *cfg;

	if ((dev == NULL) || (dev->config == NULL)) {
		cfg = NULL;
	} else {
		cfg = (const struct gpio_stm32_config *)dev->config;
	}

	return cfg;
}

static struct gpio_stm32_data *gpio_stm32_get_data(const struct device *dev)
{
	struct gpio_stm32_data *data;

	if ((dev == NULL) || (dev->data == NULL)) {
		data = NULL;
	} else {
		data = (struct gpio_stm32_data *)dev->data;
	}

	return data;
}

static uint32_t gpio_stm32_pin_ll(uint8_t pin)
{
	return ((uint32_t)1U << (uint32_t)pin);
}

static uint32_t gpio_stm32_syscfg_exti_line(uint8_t pin)
{
	uint32_t nibble;

	nibble = (uint32_t)pin % 4U;
	return (0x0FUL << ((nibble * 4U) + 16U)) | ((uint32_t)pin / 4U);
}

static IRQn_Type gpio_stm32_pin_irqn(uint8_t pin)
{
	return gpio_stm32_exti_irqn[pin];
}

static uint32_t gpio_stm32_irq_group_mask(uint8_t pin)
{
	uint32_t mask;

	if (pin <= 4U) {
		mask = gpio_stm32_pin_ll(pin);
	} else if (pin <= 9U) {
		mask = 0x03E0U;
	} else {
		mask = 0xFC00U;
	}

	return mask;
}

static void gpio_stm32_clock_on(const struct gpio_stm32_config *cfg)
{
	if (cfg != NULL) {
		LL_AHB1_GRP1_EnableClock(cfg->clock_enr);
	}
}

static int gpio_stm32_configure_pin(GPIO_TypeDef *gpio, uint8_t pin, uint32_t flags)
{
	uint32_t pin_ll;
	uint32_t mode;
	uint32_t otype;
	uint32_t pupd;
	int ret;

	pin_ll = gpio_stm32_pin_ll(pin);
	otype = LL_GPIO_OUTPUT_PUSHPULL;
	pupd = LL_GPIO_PULL_NO;
	ret = 0;

	if ((flags & GPIO_OUTPUT) != 0U) {
		mode = LL_GPIO_MODE_OUTPUT;
		if ((flags & GPIO_SINGLE_ENDED) != 0U) {
			if ((flags & GPIO_LINE_OPEN_DRAIN) != 0U) {
				otype = LL_GPIO_OUTPUT_OPENDRAIN;
			} else {
				ret = -ENOTSUP;
			}
		}
		if (ret == 0) {
			if ((flags & GPIO_PULL_UP) != 0U) {
				pupd = LL_GPIO_PULL_UP;
			} else if ((flags & GPIO_PULL_DOWN) != 0U) {
				pupd = LL_GPIO_PULL_DOWN;
			} else {
				pupd = LL_GPIO_PULL_NO;
			}
		}
	} else if ((flags & GPIO_INPUT) != 0U) {
		mode = LL_GPIO_MODE_INPUT;
		if ((flags & GPIO_PULL_UP) != 0U) {
			pupd = LL_GPIO_PULL_UP;
		} else if ((flags & GPIO_PULL_DOWN) != 0U) {
			pupd = LL_GPIO_PULL_DOWN;
		} else {
			pupd = LL_GPIO_PULL_NO;
		}
	} else {
		if (((flags & GPIO_PULL_UP) != 0U) || ((flags & GPIO_PULL_DOWN) != 0U)) {
			ret = -ENOTSUP;
		}
		mode = LL_GPIO_MODE_ANALOG;
	}

	if (ret == 0) {
		LL_GPIO_SetPinOutputType(gpio, pin_ll, otype);
		LL_GPIO_SetPinSpeed(gpio, pin_ll, LL_GPIO_SPEED_FREQ_LOW);
		LL_GPIO_SetPinPull(gpio, pin_ll, pupd);
		LL_GPIO_SetPinMode(gpio, pin_ll, mode);
	}

	return ret;
}

static int gpio_stm32_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_stm32_config *cfg;
	int ret;

	cfg = gpio_stm32_get_cfg(dev);
	if ((cfg == NULL) || (cfg->regs == NULL) || (value == NULL)) {
		ret = -EINVAL;
	} else {
		gpio_stm32_clock_on(cfg);
		*value = LL_GPIO_ReadInputPort(cfg->regs);
		ret = 0;
	}

	return ret;
}

static int gpio_stm32_port_set_masked_raw(const struct device *dev,
					  uint32_t mask,
					  uint32_t value)
{
	const struct gpio_stm32_config *cfg;
	uint32_t port_value;
	int ret;

	cfg = gpio_stm32_get_cfg(dev);
	if ((cfg == NULL) || (cfg->regs == NULL)) {
		ret = -EINVAL;
	} else {
		gpio_stm32_clock_on(cfg);
		port_value = LL_GPIO_ReadOutputPort(cfg->regs);
		LL_GPIO_WriteOutputPort(cfg->regs, (port_value & ~mask) | (mask & value));
		ret = 0;
	}

	return ret;
}

static int gpio_stm32_port_set_bits_raw(const struct device *dev, uint32_t pins)
{
	const struct gpio_stm32_config *cfg;
	int ret;

	cfg = gpio_stm32_get_cfg(dev);
	if ((cfg == NULL) || (cfg->regs == NULL)) {
		ret = -EINVAL;
	} else {
		gpio_stm32_clock_on(cfg);
		WRITE_REG(cfg->regs->BSRR, pins);
		ret = 0;
	}

	return ret;
}

static int gpio_stm32_port_clear_bits_raw(const struct device *dev, uint32_t pins)
{
	const struct gpio_stm32_config *cfg;
	int ret;

	cfg = gpio_stm32_get_cfg(dev);
	if ((cfg == NULL) || (cfg->regs == NULL)) {
		ret = -EINVAL;
	} else {
		gpio_stm32_clock_on(cfg);
		LL_GPIO_ResetOutputPin(cfg->regs, pins);
		ret = 0;
	}

	return ret;
}

static int gpio_stm32_port_toggle_bits(const struct device *dev, uint32_t pins)
{
	const struct gpio_stm32_config *cfg;
	int ret;

	cfg = gpio_stm32_get_cfg(dev);
	if ((cfg == NULL) || (cfg->regs == NULL)) {
		ret = -EINVAL;
	} else {
		gpio_stm32_clock_on(cfg);
		WRITE_REG(cfg->regs->ODR, READ_REG(cfg->regs->ODR) ^ pins);
		ret = 0;
	}

	return ret;
}

static int gpio_stm32_config(const struct device *dev, uint8_t pin, uint32_t flags)
{
	const struct gpio_stm32_config *cfg;
	int ret;

	cfg = gpio_stm32_get_cfg(dev);
	if ((cfg == NULL) || (cfg->regs == NULL) || (pin >= GPIO_STM32_PINS_PER_PORT)) {
		ret = -EINVAL;
	} else {
		gpio_stm32_clock_on(cfg);
		if ((flags & GPIO_OUTPUT) != 0U) {
			if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
				(void)gpio_stm32_port_set_bits_raw(dev, gpio_stm32_pin_ll(pin));
			} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
				(void)gpio_stm32_port_clear_bits_raw(dev, gpio_stm32_pin_ll(pin));
			} else {
				/* Keep current output level. */
			}
		}
		ret = gpio_stm32_configure_pin(cfg->regs, pin, flags);
	}

	return ret;
}

static void gpio_stm32_exti_disable_line(uint8_t pin)
{
	uint32_t line;
	IRQn_Type irqn;

	line = gpio_stm32_pin_ll(pin);
	LL_EXTI_DisableIT_0_31(line);
	LL_EXTI_DisableRisingTrig_0_31(line);
	LL_EXTI_DisableFallingTrig_0_31(line);
	LL_EXTI_ClearFlag_0_31(line);

	if ((EXTI->IMR & gpio_stm32_irq_group_mask(pin)) == 0U) {
		irqn = gpio_stm32_pin_irqn(pin);
		NVIC_DisableIRQ(irqn);
	}
}

static int gpio_stm32_pin_interrupt_configure(const struct device *dev,
					      uint8_t pin,
					      enum gpio_int_mode mode,
					      enum gpio_int_trig trig)
{
	const struct gpio_stm32_config *cfg;
	uint32_t line;
	int ret;

	cfg = gpio_stm32_get_cfg(dev);
	if ((cfg == NULL) || (pin >= GPIO_STM32_PINS_PER_PORT)) {
		ret = -EINVAL;
	} else if (mode == GPIO_INT_MODE_DISABLED) {
		if (gpio_stm32_exti_owner[pin].dev == dev) {
			gpio_stm32_exti_disable_line(pin);
			gpio_stm32_exti_owner[pin].dev = NULL;
		}
		ret = 0;
	} else if (mode == GPIO_INT_MODE_LEVEL) {
		ret = -ENOTSUP;
	} else if ((gpio_stm32_exti_owner[pin].dev != NULL) &&
		   (gpio_stm32_exti_owner[pin].dev != dev)) {
		ret = -EBUSY;
	} else if ((trig != GPIO_INT_TRIG_LOW) &&
		   (trig != GPIO_INT_TRIG_HIGH) &&
		   (trig != GPIO_INT_TRIG_BOTH)) {
		ret = -EINVAL;
	} else {
		LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
		LL_SYSCFG_SetEXTISource(cfg->port, gpio_stm32_syscfg_exti_line(pin));

		line = gpio_stm32_pin_ll(pin);
		LL_EXTI_DisableRisingTrig_0_31(line);
		LL_EXTI_DisableFallingTrig_0_31(line);
		if ((trig == GPIO_INT_TRIG_HIGH) || (trig == GPIO_INT_TRIG_BOTH)) {
			LL_EXTI_EnableRisingTrig_0_31(line);
		}
		if ((trig == GPIO_INT_TRIG_LOW) || (trig == GPIO_INT_TRIG_BOTH)) {
			LL_EXTI_EnableFallingTrig_0_31(line);
		}

		gpio_stm32_exti_owner[pin].dev = dev;
		LL_EXTI_ClearFlag_0_31(line);
		LL_EXTI_EnableIT_0_31(line);
		NVIC_EnableIRQ(gpio_stm32_pin_irqn(pin));
		ret = 0;
	}

	return ret;
}

static int gpio_stm32_manage_cb(const struct device *dev,
				struct gpio_callback *callback,
				bool set)
{
	struct gpio_stm32_data *data;
	int ret;

	data = gpio_stm32_get_data(dev);
	if (data == NULL) {
		ret = -EINVAL;
	} else {
		ret = gpio_manage_callback(&data->cb, callback, set);
	}

	return ret;
}

static uint32_t gpio_stm32_get_pending_int(const struct device *dev)
{
	const struct gpio_stm32_config *cfg;
	uint32_t pending;
	uint32_t match;
	uint8_t pin;

	cfg = gpio_stm32_get_cfg(dev);
	match = 0U;
	if (cfg != NULL) {
		pending = LL_EXTI_ReadFlag_0_31(GPIO_STM32_PIN_MASK);
		for (pin = 0U; pin < GPIO_STM32_PINS_PER_PORT; pin++) {
			if ((pending & gpio_stm32_pin_ll(pin)) != 0U) {
				if (LL_SYSCFG_GetEXTISource(gpio_stm32_syscfg_exti_line(pin)) ==
				    cfg->port) {
					match |= gpio_stm32_pin_ll(pin);
				}
			}
		}
	}

	return match;
}

static void gpio_stm32_exti_isr_range(uint8_t first, uint8_t last)
{
	uint8_t pin;
	uint32_t line;
	const struct device *port;
	struct gpio_stm32_data *data;

	for (pin = first; pin <= last; pin++) {
		line = gpio_stm32_pin_ll(pin);
		if (LL_EXTI_IsActiveFlag_0_31(line) != 0U) {
			LL_EXTI_ClearFlag_0_31(line);
			port = gpio_stm32_exti_owner[pin].dev;
			data = gpio_stm32_get_data(port);
			if (data != NULL) {
				gpio_fire_callbacks(data->cb, port, line);
			}
		}
	}
}

int gpio_stm32_init(const struct device *dev)
{
	const struct gpio_stm32_config *cfg;
	struct gpio_stm32_data *data;
	int ret;

	cfg = gpio_stm32_get_cfg(dev);
	data = gpio_stm32_get_data(dev);
	if ((cfg == NULL) || (data == NULL) || (dev->state == NULL)) {
		ret = -EINVAL;
	} else {
		data->cb = NULL;
		gpio_stm32_clock_on(cfg);
		dev->state->init_res = 0U;
		dev->state->initialized = true;
		ret = 0;
	}

	return ret;
}

static const struct gpio_driver_api gpio_stm32_driver = {
	.pin_configure = gpio_stm32_config,
	.port_get_raw = gpio_stm32_port_get_raw,
	.port_set_masked_raw = gpio_stm32_port_set_masked_raw,
	.port_set_bits_raw = gpio_stm32_port_set_bits_raw,
	.port_clear_bits_raw = gpio_stm32_port_clear_bits_raw,
	.port_toggle_bits = gpio_stm32_port_toggle_bits,
	.pin_interrupt_configure = gpio_stm32_pin_interrupt_configure,
	.manage_callback = gpio_stm32_manage_cb,
	.get_pending_int = gpio_stm32_get_pending_int
};

#define GPIO_STM32_DEFINE_PORT(_sym, _name, _regs, _port, _clk) \
	static struct gpio_stm32_data _sym##_data; \
	static struct device_state _sym##_state; \
	static const struct gpio_stm32_config _sym##_cfg = { \
		.common = { .port_pin_mask = GPIO_STM32_PIN_MASK }, \
		.regs = (_regs), \
		.port = (_port), \
		.clock_enr = (_clk) \
	}; \
	const struct device _sym = { \
		.name = (_name), \
		.config = &_sym##_cfg, \
		.api = &gpio_stm32_driver, \
		.state = &_sym##_state, \
		.data = &_sym##_data, \
		.ops = { .init = gpio_stm32_init }, \
		.flags = 0U \
	}

GPIO_STM32_DEFINE_PORT(gpioa, "GPIOA", GPIOA, 0U, LL_AHB1_GRP1_PERIPH_GPIOA);
GPIO_STM32_DEFINE_PORT(gpiob, "GPIOB", GPIOB, 1U, LL_AHB1_GRP1_PERIPH_GPIOB);
GPIO_STM32_DEFINE_PORT(gpioc, "GPIOC", GPIOC, 2U, LL_AHB1_GRP1_PERIPH_GPIOC);
GPIO_STM32_DEFINE_PORT(gpiod, "GPIOD", GPIOD, 3U, LL_AHB1_GRP1_PERIPH_GPIOD);
GPIO_STM32_DEFINE_PORT(gpioe, "GPIOE", GPIOE, 4U, LL_AHB1_GRP1_PERIPH_GPIOE);
GPIO_STM32_DEFINE_PORT(gpiof, "GPIOF", GPIOF, 5U, LL_AHB1_GRP1_PERIPH_GPIOF);
GPIO_STM32_DEFINE_PORT(gpiog, "GPIOG", GPIOG, 6U, LL_AHB1_GRP1_PERIPH_GPIOG);
GPIO_STM32_DEFINE_PORT(gpioh, "GPIOH", GPIOH, 7U, LL_AHB1_GRP1_PERIPH_GPIOH);
GPIO_STM32_DEFINE_PORT(gpioi, "GPIOI", GPIOI, 8U, LL_AHB1_GRP1_PERIPH_GPIOI);
GPIO_STM32_DEFINE_PORT(gpioj, "GPIOJ", GPIOJ, 9U, LL_AHB1_GRP1_PERIPH_GPIOJ);
GPIO_STM32_DEFINE_PORT(gpiok, "GPIOK", GPIOK, 10U, LL_AHB1_GRP1_PERIPH_GPIOK);

static const struct device *const gpio_stm32_ports[] = {
	&gpioa, &gpiob, &gpioc, &gpiod, &gpioe, &gpiof,
	&gpiog, &gpioh, &gpioi, &gpioj, &gpiok
};

void gpio_stm32_init_ports(void)
{
	uint32_t i;

	for (i = 0U; i < (sizeof(gpio_stm32_ports) / sizeof(gpio_stm32_ports[0])); i++) {
		(void)gpio_stm32_init(gpio_stm32_ports[i]);
	}
}

void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI2_IRQHandler(void);
void EXTI3_IRQHandler(void);
void EXTI4_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI15_10_IRQHandler(void);

void EXTI0_IRQHandler(void)
{
	gpio_stm32_exti_isr_range(0U, 0U);
}

void EXTI1_IRQHandler(void)
{
	gpio_stm32_exti_isr_range(1U, 1U);
}

void EXTI2_IRQHandler(void)
{
	gpio_stm32_exti_isr_range(2U, 2U);
}

void EXTI3_IRQHandler(void)
{
	gpio_stm32_exti_isr_range(3U, 3U);
}

void EXTI4_IRQHandler(void)
{
	gpio_stm32_exti_isr_range(4U, 4U);
}

void EXTI9_5_IRQHandler(void)
{
	gpio_stm32_exti_isr_range(5U, 9U);
}

void EXTI15_10_IRQHandler(void)
{
	gpio_stm32_exti_isr_range(10U, 15U);
}
