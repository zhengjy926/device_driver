/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPIO callback list helpers (trimmed from Zephyr v4.4.2).
 */

#ifndef GPIO_UTILS_H_
#define GPIO_UTILS_H_

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_DIR_MASK               (GPIO_INPUT | GPIO_OUTPUT)

#define GPIO_PUSH_PULL              ((uint32_t)0)
#define GPIO_SINGLE_ENDED           ((uint32_t)1U << 1U)
#define GPIO_LINE_OPEN_DRAIN        ((uint32_t)1U << 2U)
#define GPIO_LINE_OPEN_SOURCE       ((uint32_t)0)

#define GPIO_OUTPUT_INIT_LOW        ((uint32_t)1U << 18U)
#define GPIO_OUTPUT_INIT_HIGH       ((uint32_t)1U << 19U)
#define GPIO_OUTPUT_INIT_LOGICAL    ((uint32_t)1U << 20U)

#define GPIO_INT_ENABLE             ((uint32_t)1U << 22U)
#define GPIO_INT_LEVELS_LOGICAL     ((uint32_t)1U << 23U)
#define GPIO_INT_EDGE               ((uint32_t)1U << 24U)
#define GPIO_INT_LOW_0              ((uint32_t)1U << 25U)
#define GPIO_INT_HIGH_1             ((uint32_t)1U << 26U)

#define GPIO_INT_MASK               (GPIO_INT_DISABLE | GPIO_INT_ENABLE | \
				                     GPIO_INT_LEVELS_LOGICAL | GPIO_INT_EDGE | \
				                     GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)


/**
 * @brief Insert or remove a callback from a singly-linked list.
 *
 * @param callbacks List head.
 * @param callback Callback object.
 * @param set true to insert, false to remove.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument or callback not found on remove.
 */
static inline int gpio_manage_callback(struct gpio_callback **callbacks,
				       struct gpio_callback *callback,
				       bool set)
{
	struct gpio_callback *cur;
	struct gpio_callback *prev;
	int ret;

	if ((callbacks == NULL) || (callback == NULL) || (callback->handler == NULL)) {
		ret = -EINVAL;
	} else {
		prev = NULL;
		cur = *callbacks;
		while ((cur != NULL) && (cur != callback)) {
			prev = cur;
			cur = cur->next;
		}

		if (cur != NULL) {
			if (prev == NULL) {
				*callbacks = cur->next;
			} else {
				prev->next = cur->next;
			}
			callback->next = NULL;
			ret = 0;
		} else if (!set) {
			ret = -EINVAL;
		} else {
			ret = 0;
		}

		if ((ret == 0) && set) {
			callback->next = *callbacks;
			*callbacks = callback;
		}
	}

	return ret;
}

/**
 * @brief Invoke callbacks whose pin mask intersects @p pins.
 *
 * @param list Callback list head.
 * @param port GPIO port device.
 * @param pins Pins that triggered the interrupt.
 */
static inline void gpio_fire_callbacks(struct gpio_callback *list,
				       const struct device *port,
				       uint32_t pins)
{
	struct gpio_callback *cb;
	struct gpio_callback *next;
	uint32_t match;

	cb = list;
	while (cb != NULL) {
		next = cb->next;
		match = cb->pin_mask & pins;
		if ((match != 0U) && (cb->handler != NULL)) {
			cb->handler(port, cb, match);
		}
		cb = next;
	}
}

#ifdef __cplusplus
}
#endif

#endif /* GPIO_UTILS_H_ */
