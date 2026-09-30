/*
 * Copyright (c) 2019-2020 Nordic Semiconductor ASA
 * Copyright (c) 2019 Piotr Mienkowski
 * Copyright (c) 2017 ARM Ltd
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPIO driver API (trimmed from Zephyr, no RTOS/DT/syscall).
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <errno_def.h>
#include <limits.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "device.h"
#include "gpio_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_MAX_PINS_PER_PORT \
	((uint8_t)(sizeof(uint32_t) * (size_t)CHAR_BIT))

/** Pin is active-low (logical 1 == physical low). */
#define GPIO_ACTIVE_LOW             ((uint32_t)1U << 0U)
/** Pin is active-high (default). */
#define GPIO_ACTIVE_HIGH            ((uint32_t)0U)

/**
 * @name GPIO pin drive flags
 * @{
 */
#define GPIO_OPEN_DRAIN             (GPIO_SINGLE_ENDED | GPIO_LINE_OPEN_DRAIN)
#define GPIO_OPEN_SOURCE            (GPIO_SINGLE_ENDED | GPIO_LINE_OPEN_SOURCE)
/** @} */

/**
 * @name GPIO pin bias flags
 * @{
 */

#define GPIO_PULL_UP                ((uint32_t)1U << 4U)
#define GPIO_PULL_DOWN              ((uint32_t)1U << 5U)

/** @} */

#define GPIO_INT_WAKEUP             ((uint32_t)1U << 6U)

/**
 * @name GPIO input/output configuration flags
 * @{
 */

#define GPIO_INPUT                  ((uint32_t)1U << 16U) /** Enables pin as input. */
#define GPIO_OUTPUT                 ((uint32_t)1U << 17U) /** Enables pin as output, no change to the output state. */
#define GPIO_DISCONNECTED           ((uint32_t)0)       /** Disables pin for both input and output. */

/** Configures GPIO pin as output and initializes it to a low state. */
#define GPIO_OUTPUT_LOW             (GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW)
/** Configures GPIO pin as output and initializes it to a high state. */
#define GPIO_OUTPUT_HIGH            (GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH)
/** Configures GPIO pin as output and initializes it to a logic 0. */
#define GPIO_OUTPUT_INACTIVE        (GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_LOGICAL)
/** Configures GPIO pin as output and initializes it to a logic 1. */
#define GPIO_OUTPUT_ACTIVE          (GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOGICAL)

/** @} */

/**
 * @name GPIO interrupt configuration flags
 * @{
 */

#define GPIO_INT_DISABLE            ((uint32_t)1U << 21U) /** Disables GPIO pin interrupt. */

/** 上升沿触发，并使能中断 */
#define GPIO_INT_EDGE_RISING        (GPIO_INT_ENABLE | GPIO_INT_EDGE | \
                                     GPIO_INT_HIGH_1)
/** 下降沿触发，并使能中断 */
#define GPIO_INT_EDGE_FALLING       (GPIO_INT_ENABLE | GPIO_INT_EDGE | \
                                     GPIO_INT_LOW_0)
/** 双边沿触发，并使能中断 */
#define GPIO_INT_EDGE_BOTH          (GPIO_INT_ENABLE | GPIO_INT_EDGE | \
                                     GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)
/** 物理低电平触发，并使能中断 */
#define GPIO_INT_LEVEL_LOW          (GPIO_INT_ENABLE | GPIO_INT_LOW_0)
/** 物理高电平触发，并使能中断 */
#define GPIO_INT_LEVEL_HIGH         (GPIO_INT_ENABLE | GPIO_INT_HIGH_1)

/** 边沿跳变到逻辑0触发，并使能中断 */
#define GPIO_INT_EDGE_TO_INACTIVE   (GPIO_INT_ENABLE | GPIO_INT_LEVELS_LOGICAL | \
				                     GPIO_INT_EDGE | GPIO_INT_LOW_0)
/** 边沿跳变到逻辑1触发，并使能中断 */
#define GPIO_INT_EDGE_TO_ACTIVE     (GPIO_INT_ENABLE | GPIO_INT_LEVELS_LOGICAL | \
				                     GPIO_INT_EDGE | GPIO_INT_HIGH_1)
/** 逻辑0电平触发，并使能中断 */
#define GPIO_INT_LEVEL_INACTIVE     (GPIO_INT_ENABLE | GPIO_INT_LEVELS_LOGICAL | \
				                     GPIO_INT_LOW_0)
/** 逻辑1电平触发，并使能中断 */
#define GPIO_INT_LEVEL_ACTIVE       (GPIO_INT_ENABLE | GPIO_INT_LEVELS_LOGICAL | \
				                     GPIO_INT_HIGH_1)

/** @} */

struct gpio_driver_config {
	uint32_t port_pin_mask;
};

struct gpio_driver_data {
	uint32_t invert;
};

struct gpio_callback;

typedef void (*gpio_callback_handler_t)(const struct device *port,
					                    struct gpio_callback *cb,
					                    uint32_t pins);

struct gpio_callback {
	struct gpio_callback *next;
	gpio_callback_handler_t handler;
	uint32_t pin_mask;
};

enum gpio_int_mode {
	GPIO_INT_MODE_DISABLED = GPIO_INT_DISABLE,
	GPIO_INT_MODE_LEVEL    = GPIO_INT_ENABLE,
	GPIO_INT_MODE_EDGE     = GPIO_INT_ENABLE | GPIO_INT_EDGE,
};

enum gpio_int_trig {
	GPIO_INT_TRIG_LOW  = GPIO_INT_LOW_0,
	GPIO_INT_TRIG_HIGH = GPIO_INT_HIGH_1,
	GPIO_INT_TRIG_BOTH = GPIO_INT_LOW_0 | GPIO_INT_HIGH_1,
	GPIO_INT_TRIG_WAKE = GPIO_INT_WAKEUP,
	GPIO_INT_TRIG_WAKE_LOW = GPIO_INT_LOW_0 | GPIO_INT_WAKEUP,
	GPIO_INT_TRIG_WAKE_HIGH = GPIO_INT_HIGH_1 | GPIO_INT_WAKEUP,
	GPIO_INT_TRIG_WAKE_BOTH = GPIO_INT_LOW_0 | GPIO_INT_HIGH_1 | GPIO_INT_WAKEUP,
};

struct gpio_driver_api {
	int (*pin_configure)(const struct device *port, uint8_t pin, uint32_t flags);
	int (*port_get_raw)(const struct device *port, uint32_t *value);
	int (*port_set_masked_raw)(const struct device *port, uint32_t mask, uint32_t value);
	int (*port_set_bits_raw)(const struct device *port, uint32_t pins);
	int (*port_clear_bits_raw)(const struct device *port, uint32_t pins);
	int (*port_toggle_bits)(const struct device *port, uint32_t pins);
	int (*pin_interrupt_configure)(const struct device *port,
				                   uint8_t pin,
				                   enum gpio_int_mode mode,
				                   enum gpio_int_trig trig);
	int (*manage_callback)(const struct device *port,
			               struct gpio_callback *cb,
			               bool set);
	uint32_t (*get_pending_int)(const struct device *dev);
};

/**
 * @brief Configure pin interrupt.
 *
 * @note This function can also be used to configure interrupts on pins
 *       not controlled directly by the GPIO module. That is, pins which are
 *       routed to other modules such as I2C, SPI, UART.
 *
 * @isr_ok
 *
 * @param[in] port Pointer to device structure for the driver instance.
 * @param[in] pin Pin number.
 * @param[in] flags Interrupt configuration flags as defined by GPIO_INT_*.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If the operation is not implemented by the driver.
 * @retval -ENOTSUP If any of the configuration options is not supported
 *                  (unless otherwise directed by flag documentation).
 * @retval -EINVAL  Invalid argument.
 * @retval -EBUSY   Interrupt line required to configure pin interrupt is
 *                  already in use.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_interrupt_configure(const struct device *port,
					                           uint8_t pin,
					                           uint32_t flags)
{
	const struct gpio_driver_api *api;
	const struct gpio_driver_data *data;
	enum gpio_int_trig trig;
	enum gpio_int_mode mode;
	int ret;
    
    if ((port == NULL) || (port->api == NULL) || (port->data == NULL)) {
        ret = -EINVAL;
    }

	api = (const struct gpio_driver_api *)port->api;
	data = (const struct gpio_driver_data *)port->data;
    
    if (api->pin_interrupt_configure == NULL) {
        return -ENOSYS;
    }
    
	if (((flags & GPIO_INT_LEVELS_LOGICAL) != 0) &&
	    ((data->invert & ((uint32_t)1U << (uint32_t)pin)) != 0)) {
		/* Invert signal bits */
		flags ^= (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1);
	}

    trig = (enum gpio_int_trig)(flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1 | GPIO_INT_WAKEUP));
    mode = (enum gpio_int_mode)(flags & (GPIO_INT_EDGE | GPIO_INT_DISABLE | GPIO_INT_ENABLE));
    
    ret = api->pin_interrupt_configure(port, pin, mode, trig);
    return ret;
}

/**
 * @brief Configure a single pin.
 *
 * @param port GPIO port device.
 * @param pin Pin index.
 * @param flags Direction, bias, drive and initial-level flags.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_pin_configure(const struct device *port,
				                     uint8_t pin,
				                     uint32_t flags)
{
	const struct gpio_driver_api *api;
	struct gpio_driver_data *data;
	int ret;
    
    if ((port == NULL) || (port->api == NULL) || (port->data == NULL)) {
        ret = -EINVAL;
    }

	api = (const struct gpio_driver_api *)port->api;
	data = (struct gpio_driver_data *)port->data;
    
	if (((flags & GPIO_OUTPUT_INIT_LOGICAL) != 0)
	    && ((flags & (GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH)) != 0)
	    && ((flags & GPIO_ACTIVE_LOW) != 0)) {
		flags ^= GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH;
	}

	flags &= ~GPIO_OUTPUT_INIT_LOGICAL;

	if ((flags & GPIO_ACTIVE_LOW) != 0) {
		data->invert |= ((uint32_t)1U << (uint32_t)pin);
	} else {
		data->invert &= ~((uint32_t)1U << (uint32_t)pin);
	}

	ret = api->pin_configure(port, pin, flags);
	return ret;
}

/**
 * @brief Get physical levels of all input pins in a port.
 *
 * @param port GPIO port device.
 * @param value Storage for the port value.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_port_get_raw(const struct device *port,
				                    uint32_t *value)
{
	const struct gpio_driver_api *api;
	int ret;
    
    if ((port == NULL) || (port->api == NULL)) {
        ret = -EINVAL;
    }
    
	api = (const struct gpio_driver_api *)port->api;
    ret = api->port_get_raw(port, value);
	return ret;
}

/**
 * @brief Get logical levels of all input pins in a port.
 *
 * @param port GPIO port device.
 * @param value Storage for the port value.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_port_get(const struct device *port,
				uint32_t *value)
{
	const struct gpio_driver_data *data;
	int ret;

	if ((port == NULL) || (port->api == NULL) ||
	    (port->data == NULL) || (value == NULL)) {
		ret = -EINVAL;
	} else {
		data = (const struct gpio_driver_data *)port->data;
		ret = gpio_port_get_raw(port, value);
		if (ret == 0) {
			*value ^= data->invert;
		}
	}

	return ret;
}

/**
 * @brief Set physical levels of selected output pins.
 *
 * @param port GPIO port device.
 * @param mask Pins to modify.
 * @param value Physical levels to write.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_port_set_masked_raw(const struct device *port,
					   uint32_t mask,
					   uint32_t value)
{
	const struct gpio_driver_api *api;
	int ret;

	if ((port == NULL) || (port->api == NULL)) {
		ret = -EINVAL;
	} else {
		api = (const struct gpio_driver_api *)port->api;
		if (api->port_set_masked_raw == NULL) {
			ret = -ENOSYS;
		} else {
			ret = api->port_set_masked_raw(port, mask, value);
		}
	}

	return ret;
}

/**
 * @brief Set logical levels of selected output pins.
 *
 * @param port GPIO port device.
 * @param mask Pins to modify.
 * @param value Logical levels to write.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_port_set_masked(const struct device *port,
				       uint32_t mask,
				       uint32_t value)
{
	const struct gpio_driver_data *data;
	int ret;

	if ((port == NULL) || (port->data == NULL)) {
		ret = -EINVAL;
	} else {
		data = (const struct gpio_driver_data *)port->data;
		value ^= data->invert;
		ret = gpio_port_set_masked_raw(port, mask, value);
	}

	return ret;
}

/**
 * @brief Set selected output pins to physical high.
 *
 * @param port GPIO port device.
 * @param pins Pins to set.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_port_set_bits_raw(const struct device *port,
					 uint32_t pins)
{
	const struct gpio_driver_api *api;
	int ret;

	if ((port == NULL) || (port->api == NULL)) {
		ret = -EINVAL;
	} else {
		api = (const struct gpio_driver_api *)port->api;
		if (api->port_set_bits_raw == NULL) {
			ret = -ENOSYS;
		} else {
			ret = api->port_set_bits_raw(port, pins);
		}
	}

	return ret;
}

/**
 * @brief Set selected output pins to logical active.
 *
 * @param port GPIO port device.
 * @param pins Pins to set.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_port_set_bits(const struct device *port,
				     uint32_t pins)
{
	return gpio_port_set_masked(port, pins, pins);
}

/**
 * @brief Set selected output pins to physical low.
 *
 * @param port GPIO port device.
 * @param pins Pins to clear.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_port_clear_bits_raw(const struct device *port,
					   uint32_t pins)
{
	const struct gpio_driver_api *api;
	int ret;

	if ((port == NULL) || (port->api == NULL)) {
		ret = -EINVAL;
	} else {
		api = (const struct gpio_driver_api *)port->api;
		if (api->port_clear_bits_raw == NULL) {
			ret = -ENOSYS;
		} else {
			ret = api->port_clear_bits_raw(port, pins);
		}
	}

	return ret;
}

/**
 * @brief Set selected output pins to logical inactive.
 *
 * @param port GPIO port device.
 * @param pins Pins to clear.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_port_clear_bits(const struct device *port,
				       uint32_t pins)
{
	return gpio_port_set_masked(port, pins, 0U);
}

/**
 * @brief Toggle selected output pins.
 *
 * @param port GPIO port device.
 * @param pins Pins to toggle.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_port_toggle_bits(const struct device *port,
					uint32_t pins)
{
	const struct gpio_driver_api *api;
	int ret;

	if ((port == NULL) || (port->api == NULL)) {
		ret = -EINVAL;
	} else {
		api = (const struct gpio_driver_api *)port->api;
		if (api->port_toggle_bits == NULL) {
			ret = -ENOSYS;
		} else {
			ret = api->port_toggle_bits(port, pins);
		}
	}

	return ret;
}

/**
 * @brief Set and clear physical levels of selected output pins.
 *
 * @param port GPIO port device.
 * @param set_pins Pins to set high.
 * @param clear_pins Pins to set low.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_port_set_clr_bits_raw(const struct device *port,
					     uint32_t set_pins,
					     uint32_t clear_pins)
{
	int ret;

	if ((set_pins & clear_pins) != 0U) {
		ret = -EINVAL;
	} else {
		ret = gpio_port_set_masked_raw(port, set_pins | clear_pins, set_pins);
	}

	return ret;
}

/**
 * @brief Set and clear logical levels of selected output pins.
 *
 * @param port GPIO port device.
 * @param set_pins Pins to set active.
 * @param clear_pins Pins to set inactive.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_port_set_clr_bits(const struct device *port,
					 uint32_t set_pins,
					 uint32_t clear_pins)
{
	int ret;

	if ((set_pins & clear_pins) != 0U) {
		ret = -EINVAL;
	} else {
		ret = gpio_port_set_masked(port, set_pins | clear_pins, set_pins);
	}

	return ret;
}

/**
 * @brief Get the physical level of an input pin.
 *
 * @param port GPIO port device.
 * @param pin Pin index.
 *
 * @retval 1 Physical high.
 * @retval 0 Physical low.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_pin_get_raw(const struct device *port, uint8_t pin)
{
	uint32_t value;
	uint32_t pin_mask;
	int ret;

	if ((port == NULL) || (port->api == NULL) ||
	    (pin >= GPIO_MAX_PINS_PER_PORT)) {
		ret = -EINVAL;
	} else {
		pin_mask = ((uint32_t)1U << (uint32_t)pin);
		ret = gpio_port_get_raw(port, &value);
		if (ret == 0) {
			if ((value & pin_mask) != 0U) {
				ret = 1;
			} else {
				ret = 0;
			}
		}
	}

	return ret;
}

/**
 * @brief Get the logical level of an input pin.
 *
 * @param port GPIO port device.
 * @param pin Pin index.
 *
 * @retval 1 Logical active.
 * @retval 0 Logical inactive.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_pin_get(const struct device *port, uint8_t pin)
{
	uint32_t value;
	uint32_t pin_mask;
	int ret;

	if ((port == NULL) || (port->api == NULL) || (port->data == NULL) ||
	    (pin >= GPIO_MAX_PINS_PER_PORT)) {
		ret = -EINVAL;
	} else {
		pin_mask = ((uint32_t)1U << (uint32_t)pin);
		ret = gpio_port_get(port, &value);
		if (ret == 0) {
			if ((value & pin_mask) != 0U) {
				ret = 1;
			} else {
				ret = 0;
			}
		}
	}

	return ret;
}

/**
 * @brief Set the physical level of an output pin.
 *
 * @param port GPIO port device.
 * @param pin Pin index.
 * @param value Non-zero for high, zero for low.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_pin_set_raw(const struct device *port, uint8_t pin,
				   int value)
{
	uint32_t pin_mask;
	int ret;

	if ((port == NULL) || (port->api == NULL) ||
	    (pin >= GPIO_MAX_PINS_PER_PORT)) {
		ret = -EINVAL;
	} else {
		pin_mask = ((uint32_t)1U << (uint32_t)pin);
		if (value != 0) {
			ret = gpio_port_set_bits_raw(port, pin_mask);
		} else {
			ret = gpio_port_clear_bits_raw(port, pin_mask);
		}
	}

	return ret;
}

/**
 * @brief Set the logical level of an output pin.
 *
 * @param port GPIO port device.
 * @param pin Pin index.
 * @param value Non-zero for active, zero for inactive.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_pin_set(const struct device *port, uint8_t pin,
			       int value)
{
	const struct gpio_driver_data *data;
	uint32_t pin_mask;
	int physical;
	int ret;

	if ((port == NULL) || (port->data == NULL) ||
	    (pin >= GPIO_MAX_PINS_PER_PORT)) {
		ret = -EINVAL;
	} else {
		data = (const struct gpio_driver_data *)port->data;
		pin_mask = ((uint32_t)1U << (uint32_t)pin);
		physical = value;
		if ((data->invert & pin_mask) != 0U) {
			if (value != 0) {
				physical = 0;
			} else {
				physical = 1;
			}
		}
		ret = gpio_pin_set_raw(port, pin, physical);
	}

	return ret;
}

/**
 * @brief Toggle an output pin.
 *
 * @param port GPIO port device.
 * @param pin Pin index.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_pin_toggle(const struct device *port, uint8_t pin)
{
	int ret;

	if ((port == NULL) || (port->api == NULL) ||
	    (pin >= GPIO_MAX_PINS_PER_PORT)) {
		ret = -EINVAL;
	} else {
		ret = gpio_port_toggle_bits(port, ((uint32_t)1U << (uint32_t)pin));
	}

	return ret;
}

/**
 * @brief Initialize a GPIO callback object.
 *
 * @param callback Callback object (must not be on the stack).
 * @param handler Handler function.
 * @param pin_mask Pins of interest.
 */
static inline void gpio_init_callback(struct gpio_callback *callback,
				      gpio_callback_handler_t handler,
				      uint32_t pin_mask)
{
	if ((callback != NULL) && (handler != NULL)) {
		callback->next = NULL;
		callback->handler = handler;
		callback->pin_mask = pin_mask;
	}
}

/**
 * @brief Register an application GPIO callback.
 *
 * @param port GPIO port device.
 * @param callback Initialized callback object.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_add_callback(const struct device *port,
				    struct gpio_callback *callback)
{
	const struct gpio_driver_api *api;
	int ret;

	if ((port == NULL) || (port->api == NULL) || (callback == NULL) ||
	    (callback->handler == NULL)) {
		ret = -EINVAL;
	} else {
		api = (const struct gpio_driver_api *)port->api;
		if (api->manage_callback == NULL) {
			ret = -ENOSYS;
		} else {
			ret = api->manage_callback(port, callback, true);
		}
	}

	return ret;
}

/**
 * @brief Unregister an application GPIO callback.
 *
 * @param port GPIO port device.
 * @param callback Previously registered callback object.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_remove_callback(const struct device *port,
				       struct gpio_callback *callback)
{
	const struct gpio_driver_api *api;
	int ret;

	if ((port == NULL) || (port->api == NULL) || (callback == NULL)) {
		ret = -EINVAL;
	} else {
		api = (const struct gpio_driver_api *)port->api;
		if (api->manage_callback == NULL) {
			ret = -ENOSYS;
		} else {
			ret = api->manage_callback(port, callback, false);
		}
	}

	return ret;
}

/**
 * @brief Get pending GPIO interrupt status.
 *
 * @param dev GPIO port device.
 *
 * @return Non-zero if at least one interrupt is pending, 0 if none.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSYS Operation not implemented.
 */
static inline int gpio_get_pending_int(const struct device *dev)
{
	const struct gpio_driver_api *api;
	int ret;

	if ((dev == NULL) || (dev->api == NULL)) {
		ret = -EINVAL;
	} else {
		api = (const struct gpio_driver_api *)dev->api;
		if (api->get_pending_int == NULL) {
			ret = -ENOSYS;
		} else {
			ret = (int)api->get_pending_int(dev);
		}
	}

	return ret;
}

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H_ */
