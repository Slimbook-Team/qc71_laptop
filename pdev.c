// SPDX-License-Identifier: GPL-2.0
#include "pr.h"

#include <linux/bug.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

#include "util.h"
#include "ec.h"
#include "features.h"
#include "misc.h"
#include "pdev.h"

/* ========================================================================== */

struct platform_device *qc71_platform_dev;

/* ========================================================================== */

static ssize_t fan_reduced_duty_cycle_show(struct device *dev,
					   struct device_attribute *attr, char *buf)
{
	int status = ec_read_byte(BIOS_CTRL_3_ADDR);

	if (status < 0)
		return status;

	return sprintf(buf, "%d\n", !!(status & BIOS_CTRL_3_FAN_REDUCED_DUTY_CYCLE));
}

static ssize_t fan_reduced_duty_cycle_store(struct device *dev, struct device_attribute *attr,
					    const char *buf, size_t count)
{
	int status;
	bool value;

	if (kstrtobool(buf, &value))
		return -EINVAL;

	status = ec_read_byte(BIOS_CTRL_3_ADDR);
	if (status < 0)
		return status;

	status = SET_BIT(status, BIOS_CTRL_3_FAN_REDUCED_DUTY_CYCLE, value);

	status = ec_write_byte(BIOS_CTRL_3_ADDR, status);

	if (status < 0)
		return status;

	return count;
}

static ssize_t fan_always_on_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int status = ec_read_byte(BIOS_CTRL_3_ADDR);

	if (status < 0)
		return status;

	return sprintf(buf, "%d\n", !!(status & BIOS_CTRL_3_FAN_ALWAYS_ON));
}

static ssize_t fan_always_on_store(struct device *dev, struct device_attribute *attr,
				   const char *buf, size_t count)
{
	int status;
	bool value;

	if (kstrtobool(buf, &value))
		return -EINVAL;

	status = ec_read_byte(BIOS_CTRL_3_ADDR);
	if (status < 0)
		return status;

	status = SET_BIT(status, BIOS_CTRL_3_FAN_ALWAYS_ON, value);

	status = ec_write_byte(BIOS_CTRL_3_ADDR, status);

	if (status < 0)
		return status;

	return count;
}

static ssize_t fn_lock_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	int status = qc71_fn_lock_get_state();

	if (status < 0)
		return status;

	return sprintf(buf, "%d\n", status);
}

static ssize_t fn_lock_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int status;
	bool value;

	if (kstrtobool(buf, &value))
		return -EINVAL;

	status = qc71_fn_lock_set_state(value);
	if (status < 0)
		return status;

	return count;
}

static ssize_t fn_lock_switch_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int status = ec_read_byte(AP_BIOS_BYTE_ADDR);

	if (status < 0)
		return status;

	return sprintf(buf, "%d\n", !!(status & AP_BIOS_BYTE_FN_LOCK_SWITCH));
}

static ssize_t fn_lock_switch_store(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int status;
	bool value;

	if (kstrtobool(buf, &value))
		return -EINVAL;

	status = ec_read_byte(AP_BIOS_BYTE_ADDR);
	if (status < 0)
		return status;

	status = SET_BIT(status, AP_BIOS_BYTE_FN_LOCK_SWITCH, value);

	status = ec_write_byte(AP_BIOS_BYTE_ADDR, status);

	if (status < 0)
		return status;

	return count;
}

static ssize_t manual_control_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int status = ec_read_byte(CTRL_1_ADDR);

	if (status < 0)
		return status;

	return sprintf(buf, "%d\n", !!(status & CTRL_1_MANUAL_MODE));
}

static ssize_t manual_control_store(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int status;
	bool value;

	if (kstrtobool(buf, &value))
		return -EINVAL;

	status = ec_read_byte(CTRL_1_ADDR);
	if (status < 0)
		return status;

	status = SET_BIT(status, CTRL_1_MANUAL_MODE, value);

	status = ec_write_byte(CTRL_1_ADDR, status);

	if (status < 0)
		return status;

	return count;
}

static ssize_t super_key_lock_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int status = ec_read_byte(STATUS_1_ADDR);

	if (status < 0)
		return status;

	return sprintf(buf, "%d\n", !!(status & STATUS_1_SUPER_KEY_LOCK));
}

static ssize_t super_key_lock_store(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int status;
	bool value;

	if (kstrtobool(buf, &value))
		return -EINVAL;

	status = ec_read_byte(STATUS_1_ADDR);
	if (status < 0)
		return status;

	if (value != !!(status & STATUS_1_SUPER_KEY_LOCK)) {
		status = ec_write_byte(TRIGGER_1_ADDR, TRIGGER_1_SUPER_KEY_LOCK);

		if (status < 0)
			return status;
	}

	return count;
}

static ssize_t silent_mode_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int status = ec_read_byte(FAN_CTRL_ADDR);

	if (status < 0)
		return status;

	return sprintf(buf, "%d\n", !!(status & FAN_CTRL_SILENT_MODE));
}

static ssize_t silent_mode_store(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int status;
	bool value;

	if (kstrtobool(buf, &value))
		return -EINVAL;

	status = ec_read_byte(FAN_CTRL_ADDR);
	if (status < 0)
		return status;

	status = SET_BIT(status, FAN_CTRL_SILENT_MODE, value);

	status = ec_write_byte(FAN_CTRL_ADDR, status);

	if (status < 0)
		return status;

	return count;
}

static ssize_t turbo_mode_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int status = ec_read_byte(FAN_CTRL_ADDR);

	if (status < 0)
		return status;

	return sprintf(buf, "%d\n", !!(status & FAN_CTRL_TURBO));
}

static ssize_t turbo_mode_store(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int status;
	bool value;

	if (kstrtobool(buf, &value))
		return -EINVAL;

	status = ec_read_byte(FAN_CTRL_ADDR);
	if (status < 0)
		return status;

	status = SET_BIT(status, FAN_CTRL_TURBO, value);

	status = ec_write_byte(FAN_CTRL_ADDR, status);

	if (status < 0)
		return status;

	return count;
}

static ssize_t performance_mode_show(struct device *dev,
							   struct device_attribute *attr, char *buf)
{
	int status = ec_read_byte(FAN_CTRL_ADDR);

	if (status < 0)
		return status;

	status = status & (FAN_CTRL_TURBO | FAN_CTRL_SILENT_MODE);
	int mode = 0;

	switch (status) {
		case 0:
			mode = 2;
		break;

		case FAN_CTRL_SILENT_MODE:
			mode = 1;
		break;

		case FAN_CTRL_TURBO:
			mode = 3;
		break;

		default:
			mode = -1;
	}

	return sprintf(buf, "%d\n", mode);
}

static ssize_t performance_mode_store(struct device *dev, struct device_attribute *attr,
								const char *buf, size_t count)
{
	int status;
	uint32_t value = 0;

	if (kstrtouint(buf,0, &value))
		return -EINVAL;

	status = ec_read_byte(FAN_CTRL_ADDR);
	if (status < 0)
		return status;

	status = SET_BIT(status, FAN_CTRL_TURBO, value == 3);
	status = SET_BIT(status, FAN_CTRL_SILENT_MODE, value == 1);

	status = ec_write_byte(FAN_CTRL_ADDR, status);

	if (status < 0)
		return status;

	return count;
}

static ssize_t custom_tdp_show(struct device *dev,
							   struct device_attribute *attr, char *buf)
{
	int pl1,pl2,pl4;
	int status = ec_read_byte(PL1_ADDR);
	
	if (status < 0)
		return status;
	
	pl1 = status;
	
	status = ec_read_byte(PL2_ADDR);
	
	if (status < 0)
		return status;
	
	pl2 = status;
	
	status = ec_read_byte(PL4_ADDR);
	
	if (status < 0)
		return status;
	
	pl4 = status;
	
	return sprintf(buf, "%d %d %d\n", pl1, pl2, pl4);
}

static ssize_t custom_tdp_store(struct device *dev, struct device_attribute *attr,
								const char *buf, size_t count)
{
	char* found;
	int pl[3];
	int num = 0;
	int value;
	int status;
	
	while( (found = strsep((char**)&buf," ")) != NULL && num < 3) {
		if (kstrtouint(found,0, &value))
			return -EINVAL;
		pl[num] = value;
		num++;
	}
	
	if (num == 3) {
		status = ec_write_byte(PL1_ADDR, pl[0]);

		if (status < 0)
			return status;
		
		status = ec_write_byte(PL2_ADDR, pl[1]);

		if (status < 0)
			return status;
		
		status = ec_write_byte(PL4_ADDR, pl[2]);

		if (status < 0)
			return status;
		
		if (pl[0] == 0 && pl[1] == 0 && pl[2] == 0) {
			status = ec_read_byte(CTRL_7_ADDR);
			
			if (status < 0)
				return status;
			
			status = SET_BIT(status, CTRL_7_CUSTOM_MODE, 0);
			ec_write_byte(CTRL_7_ADDR, status);
		}
		else {
			status = ec_read_byte(CTRL_7_ADDR);
			
			if (status < 0)
				return status;
			
			status = SET_BIT(status, CTRL_7_CUSTOM_MODE, 1);
			ec_write_byte(CTRL_7_ADDR, status);
		}
	}
	
	return count;
}

static ssize_t custom_mode_show(struct device *dev,
							   struct device_attribute *attr, char *buf)
{
	int status = ec_read_byte(CTRL_7_ADDR);

	if (status < 0)
		return status;

	return sprintf(buf, "%d\n", !!(status & CTRL_7_CUSTOM_MODE));
}

static ssize_t ac_auto_boot_show(struct device *dev,
							   struct device_attribute *attr, char *buf)
{

	int status = ec_read_byte(CTRL_9_ADDR);

	if (status < 0)
		return status;

	return (status & CTRL_9_AC_AUTO_BOOT_ENABLE) != 0;
}

static ssize_t ac_auto_boot_store(struct device *dev, struct device_attribute *attr,
								const char *buf, size_t count)
{
	int status;
	uint32_t value = 0;

	if (kstrtouint(buf,0, &value))
		return -EINVAL;

	status = ec_read_byte(CTRL_9_ADDR);
	if (status < 0)
		return status;

	status = SET_BIT(status, CTRL_9_AC_AUTO_BOOT_ENABLE, value == 1);

	status = ec_write_byte(CTRL_9_ADDR, status);

	if (status < 0)
		return status;

	return count;
}

/* ========================================================================== */

static DEVICE_ATTR_RW(fn_lock);
static DEVICE_ATTR_RW(fn_lock_switch);
static DEVICE_ATTR_RW(fan_always_on);
static DEVICE_ATTR_RW(fan_reduced_duty_cycle);
static DEVICE_ATTR_RW(manual_control);
static DEVICE_ATTR_RW(super_key_lock);
static DEVICE_ATTR_RW(silent_mode);
static DEVICE_ATTR_RW(turbo_mode);
static DEVICE_ATTR_RW(performance_mode);
static DEVICE_ATTR_RW(custom_tdp);
static DEVICE_ATTR_RO(custom_mode);
static DEVICE_ATTR_RW(ac_auto_boot);

static struct attribute *qc71_laptop_attrs[] = {
	&dev_attr_fn_lock.attr,
	&dev_attr_fn_lock_switch.attr,
	&dev_attr_fan_always_on.attr,
	&dev_attr_fan_reduced_duty_cycle.attr,
	&dev_attr_manual_control.attr,
	&dev_attr_super_key_lock.attr,
	&dev_attr_silent_mode.attr,
	&dev_attr_turbo_mode.attr,
	&dev_attr_performance_mode.attr,
	&dev_attr_custom_tdp.attr,
	&dev_attr_custom_mode.attr,
	&dev_attr_ac_auto_boot.attr,
	NULL
};

/* ========================================================================== */

static umode_t qc71_laptop_attr_is_visible(struct kobject *kobj, struct attribute *attr, int n)
{
	bool ok = false;

	if (attr == &dev_attr_fn_lock.attr || attr == &dev_attr_fn_lock_switch.attr)
		ok = qc71_features.fn_lock;
	else if (attr == &dev_attr_fan_always_on.attr || attr == &dev_attr_fan_reduced_duty_cycle.attr)
		ok = qc71_features.fan_extras;
	else if (attr == &dev_attr_manual_control.attr)
		ok = true;
	else if (attr == &dev_attr_super_key_lock.attr)
		ok = qc71_features.super_key_lock;
	else if (attr == &dev_attr_silent_mode.attr)
		ok = qc71_features.silent_mode;
	else if (attr == &dev_attr_turbo_mode.attr)
		ok = qc71_features.turbo_mode;
	else if (attr == &dev_attr_performance_mode.attr)
		ok = qc71_features.turbo_mode || qc71_features.silent_mode;
	else if (attr == &dev_attr_custom_tdp.attr)
		ok = qc71_features.turbo_mode || qc71_features.silent_mode;
	else if (attr == &dev_attr_custom_mode.attr)
		ok = qc71_features.turbo_mode || qc71_features.silent_mode;

	return ok ? attr->mode : 0;
}

/* ========================================================================== */

static const struct attribute_group qc71_laptop_group = {
	.is_visible = qc71_laptop_attr_is_visible,
	.attrs = qc71_laptop_attrs,
};

static const struct attribute_group *qc71_laptop_groups[] = {
	&qc71_laptop_group,
	NULL
};

/* ========================================================================== */

int __init qc71_pdev_setup(void)
{
	int err;

	qc71_platform_dev = platform_device_alloc(KBUILD_MODNAME, PLATFORM_DEVID_NONE);
	if (!qc71_platform_dev)
		return -ENOMEM;

	qc71_platform_dev->dev.groups = qc71_laptop_groups;

	err = platform_device_add(qc71_platform_dev);
	if (err) {
		platform_device_put(qc71_platform_dev);
		qc71_platform_dev = NULL;
	}

	return err;
}

void qc71_pdev_cleanup(void)
{
	/* checks for IS_ERR_OR_NULL() */
	platform_device_unregister(qc71_platform_dev);
}
