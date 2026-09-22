// SPDX-License-Identifier: GPL-2.0
#include "pr.h"

#include <linux/init.h>
#include <linux/led-class-multicolor.h>
#include <linux/leds.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/fixp-arith.h>
#include <linux/bitfield.h>

#include "util.h"
#include "ec.h"
#include "features.h"
#include "led_keyboard.h"
#include "pdev.h"

static bool keyboard_led_registered;
static bool keyboard_white_led_registered;

/* -1: detect from the EC, 0: never register, 1: always register */
static int kbd_white = -1;
module_param(kbd_white, int, 0444);
MODULE_PARM_DESC(kbd_white, "white keyboard backlight: -1 known Slimbook models (default), 0 off, 1 force; ignored on RGB keyboards");

static uint kbd_white_max = 4;
module_param(kbd_white_max, uint, 0444);
MODULE_PARM_DESC(kbd_white_max, "highest white keyboard backlight level (default=4)");

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
        .channel = 0
    },
    {
        .color_index = LED_COLOR_ID_GREEN,
        .intensity = 0xff,
        .channel = 0
    },
    {
        .color_index = LED_COLOR_ID_BLUE,
        .intensity = 0xff,
        .channel = 0
    }
};

static struct led_classdev_mc qc71_keyboard_led = {
    .led_cdev.name                    = "rgb:"LED_FUNCTION_KBD_BACKLIGHT,
    .led_cdev.max_brightness          = 4,
    .led_cdev.brightness_get          = qc71_keyboard_led_get_brightness,
    .led_cdev.brightness_set_blocking = qc71_keyboard_led_set_brightness,
    .led_cdev.flags                   = LED_BRIGHT_HW_CHANGED,
    .num_colors                       = 3,
    .subled_info                      = qc71_subleds
};

/* ========================================================================== */

/*
 * White keyboards keep the brightness in bits 7:5 of the status register.
 * The EC changes it itself on the Fn backlight key, so read it back rather
 * than trust a cached value: that keeps save and restore right.
 */
static enum led_brightness qc71_white_led_get_brightness(struct led_classdev *led_cdev)
{
    int data = ec_read_byte(CTRL_2_ADDR);

    if (data < 0)
        return led_cdev->brightness;

    /* the Fn key may cycle past a kbd_white_max set too low */
    return min_t(int, FIELD_GET(CTRL_2_SINGLE_COLOR_KBD_BRIGHTNESS, data),
                 led_cdev->max_brightness);
}

static int qc71_white_led_set_brightness(struct led_classdev *led_cdev,
                                         enum led_brightness value)
{
    int data, err;

    /*
     * Some white keyboards ignore a new level while the backlight is off
     * until the immediate brightness register is touched (TUXEDO's
     * uniwill_write_kbd_bl_brightness_white_workaround).
     */
    if (value) {
        data = ec_read_byte(KBD_BACKLIGHT_RGB_BLUE_ADDR);
        if (data == 0 && ec_write_byte(KBD_BACKLIGHT_RGB_BLUE_ADDR, 0x01))
            pr_debug("failed to wake the immediate brightness register\n");
    }

    data = ec_read_byte(CTRL_2_ADDR);
    if (data < 0)
        return data;

    /* keep the low nibble, set the level and the apply bit */
    data = (data & 0x0f) | FIELD_PREP(CTRL_2_SINGLE_COLOR_KBD_BRIGHTNESS, value) |
           CTRL_2_COLOR_KBD_TRIGGER;

    err = ec_write_byte(CTRL_2_ADDR, data);
    if (err)
        return err;

    led_cdev->brightness = value;

    return 0;
}

static struct led_classdev qc71_white_keyboard_led = {
    .name                    = "white:" LED_FUNCTION_KBD_BACKLIGHT,
    .brightness_get          = qc71_white_led_get_brightness,
    .brightness_set_blocking = qc71_white_led_set_brightness,
    /* keep the level when the module unloads; the LED core would switch it off */
    .flags                   = LED_RETAIN_AT_SHUTDOWN,
};

static int __init qc71_white_led_keyboard_setup(void)
{
    int err;

    if (kbd_white == 0 || (kbd_white < 0 && !qc71_features.kbd_backlight_white))
        return -ENODEV;

    qc71_white_keyboard_led.max_brightness = clamp_t(uint, kbd_white_max, 1, 7);

    err = devm_led_classdev_register(&qc71_platform_dev->dev, &qc71_white_keyboard_led);

    if (!err)
        keyboard_white_led_registered = true;

    return err;
}

/* ========================================================================== */

int __init qc71_led_keyboard_setup(void)
{
    int err;

    if (!qc71_features.kbd_backlight_rgb)
        return qc71_white_led_keyboard_setup();

    err = devm_led_classdev_multicolor_register_ext(&qc71_platform_dev->dev, &qc71_keyboard_led, NULL);

    if (!err)
        keyboard_led_registered = true;

    return err;
}

void qc71_led_keyboard_cleanup(void)
{
    if (keyboard_white_led_registered) {
        devm_led_classdev_unregister(&qc71_platform_dev->dev, &qc71_white_keyboard_led);
        keyboard_white_led_registered = false;
    }

    if (keyboard_led_registered) {
        devm_led_classdev_multicolor_unregister(&qc71_platform_dev->dev, &qc71_keyboard_led);
    }
}
