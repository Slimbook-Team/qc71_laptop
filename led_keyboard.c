// SPDX-License-Identifier: GPL-2.0
#include "pr.h"

#include <linux/init.h>
#include <linux/led-class-multicolor.h>
#include <linux/leds.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/fixp-arith.h>
#include <linux/version.h>

#include "util.h"
#include "ec.h"
#include "features.h"
#include "led_keyboard.h"
#include "pdev.h"

static bool keyboard_led_registered;

static enum led_brightness qc71_keyboard_led_get_brightness(struct led_classdev *led_cdev)
{
	return led_cdev->brightness;
}

static int qc71_keyboard_led_set_brightness(struct led_classdev *led_cdev,
											enum led_brightness value)
{
	struct led_classdev_mc *mcled_cdev = lcdev_to_mccdev(led_cdev);

	int red = mcled_cdev->subled_info[0].intensity ;
	int green = mcled_cdev->subled_info[1].intensity ;
	int blue =  mcled_cdev->subled_info[2].intensity ;

	ec_write_byte(KBD_BACKLIGHT_RGB_RED_SETUP_ADDR,fixp_linear_interpolate(0, 0, U8_MAX, 0x32, red) );
	ec_write_byte(KBD_BACKLIGHT_RGB_GREEN_SETUP_ADDR,fixp_linear_interpolate(0, 0, U8_MAX, 0x32, green) );
	ec_write_byte(KBD_BACKLIGHT_RGB_BLUE_SETUP_ADDR,fixp_linear_interpolate(0, 0, U8_MAX, 0x32, blue) );


	int data = ec_read_byte(TRIGGER_1_ADDR);
	ec_write_byte(TRIGGER_1_ADDR, data | 0x20);

	data = ec_read_byte(CTRL_2_ADDR) & 0x0f;
	data = data | (value << 5) | CTRL_2_COLOR_KBD_TRIGGER;
	ec_write_byte(CTRL_2_ADDR, data);

	led_cdev->brightness = value;

	return 0;
}

/* ========================================================================== */


static struct mc_subled qc71_subleds[3] = {
	{
		.color_index = LED_COLOR_ID_RED,
		.intensity = 0xff,
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(7, 2, 0)
		.max_intensity = 0xff,
		#endif
		.channel = 0
	},
	{
		.color_index = LED_COLOR_ID_GREEN,
		.intensity = 0xff,
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(7, 2, 0)
		.max_intensity = 0xff,
		#endif
		.channel = 0
	},
	{
		.color_index = LED_COLOR_ID_BLUE,
		.intensity = 0xff,
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(7, 2, 0)
		.max_intensity = 0xff,
		#endif
		.channel = 0
	}
};

static struct led_classdev_mc qc71_keyboard_led = {
	.led_cdev.name						= "rgb:"LED_FUNCTION_KBD_BACKLIGHT,
	.led_cdev.max_brightness			= 4,
	.led_cdev.brightness_get			= qc71_keyboard_led_get_brightness,
	.led_cdev.brightness_set_blocking	= qc71_keyboard_led_set_brightness,
	.led_cdev.flags						= LED_BRIGHT_HW_CHANGED,
	.num_colors							= 3,
	.subled_info						= qc71_subleds
};

/* ========================================================================== */

int __init qc71_led_keyboard_setup(void)
{
	int err;

	if (!qc71_features.kbd_backlight_rgb)
		return -ENODEV;

	err = devm_led_classdev_multicolor_register_ext(&qc71_platform_dev->dev, &qc71_keyboard_led, NULL);

	if (!err)
		keyboard_led_registered = true;

	return err;
}

void qc71_led_keyboard_cleanup(void)
{
	if (keyboard_led_registered) {
		devm_led_classdev_multicolor_unregister(&qc71_platform_dev->dev, &qc71_keyboard_led);
	}
}
