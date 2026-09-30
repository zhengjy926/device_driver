#include "device.h"

const struct device *device_get(const char *name)
{
	/* A null string identifies no device.  So does an empty
	 * string.
	 */
	if ((name == NULL) || (name[0] == '\0')) {
		return NULL;
	}

	/* Return NULL if the device matching 'name' is not ready. */
	STRUCT_SECTION_FOREACH(device, dev) {
		if ((dev->name == name) || (strcmp(name, dev->name) == 0)) {
			return device_is_ready(dev) ? dev : NULL;
		}
	}

	return NULL;
}

bool device_is_ready(const struct device *dev)
{
	/*
	 * if an invalid device pointer is passed as argument, this call
	 * reports the `device` as not ready for usage.
	 */
	if (dev == NULL) {
		return false;
	}

	return dev->state->initialized && (dev->state->init_res == 0U);
}
