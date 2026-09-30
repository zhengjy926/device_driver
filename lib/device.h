/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEVICE_H_
#define DEVICE_H_

#include <stdint.h>
#include <stdbool.h>

#include "init.h"
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Runtime device dynamic structure (in RAM) per driver instance
 *
 * Fields in this are expected to be default-initialized to zero. The
 * kernel driver infrastructure and driver access functions are
 * responsible for ensuring that any non-zero initialization is done
 * before they are accessed.
 */
struct device_state {
	/**
	 * Device initialization return code (positive errno value).
	 *
	 * Device initialization functions return a negative errno code if they
	 * fail. In Zephyr, errno values do not exceed 255, so we can store the
	 * positive result value in a uint8_t type.
	 */
	uint8_t init_res;

	/** Indicates the device initialization function has been
	 * invoked.
	 */
	bool initialized : 1;
};

/** Device flags */
typedef uint8_t device_flags_t;

/** Device operations */
struct device_ops {
	/** Initialization function */
	int (*init)(const struct device *dev);
};

/**
 * @brief Runtime device structure (in ROM) per driver instance
 */
struct device {
	const char          *name;  /** Name of the device instance */
	const void          *config;/** Address of device instance config information */
	const void          *api;   /** Address of the API structure exposed by the device instance */
	struct device_state *state; /** Address of the common device state */
	void                *data;  /** Address of the device instance private data */
	struct device_ops    ops;   /** Device operations */
	device_flags_t       flags; /** Device flags */
};


const struct device *device_get(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_H_ */
