/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (C) 2025 Savoir-faire Linux, Inc.
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STM32F4 GPIO driver (trimmed from Zephyr v4.4.2).
 */

#ifndef GPIO_STM32_H_
#define GPIO_STM32_H_

#include "gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const struct device gpioa;
extern const struct device gpiob;
extern const struct device gpioc;
extern const struct device gpiod;
extern const struct device gpioe;
extern const struct device gpiof;
extern const struct device gpiog;
extern const struct device gpioh;
extern const struct device gpioi;
extern const struct device gpioj;
extern const struct device gpiok;

int gpio_stm32_init(const struct device *dev);
void gpio_stm32_init_ports(void);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_STM32_H_ */
