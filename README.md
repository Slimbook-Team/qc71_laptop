# What is it?
This a Linux kernel platform driver for Intel Whitebook LAPQC71X systems (XMG Fusion 15, Eluktronics MAG 15, Aftershock Vapor 15, ...).

# Disclaimer
**This software is in early stages of developement. Futhermore, to quote GPL: everything is provided as is. There is no warranty for the program, to the extent permitted by applicable law.**

**This software is licensed under the GNU General Public License v2.0**


# Compatibility
It has only been tested on an XMG Fusion 15 device (BIOS 0062 and 0114) and with kernel version `5.4.44` and `5.4.52`.


# Dependencies
### Required
* Your kernel has been compiled with `CONFIG_ACPI` and `CONFIG_DMI` (it probably was)
* Linux headers for you current kernel

### Optional
* DKMS if you don't want to recompile it manually every time the kernel is upgraded


# Features
## Current
* Integrate fan speeds into the Linux hardware monitoring subsyste (so that `lm_sensors` can pick it up)
* Control the lightbar
* Enable/disable passive cooling, reduced fan duty cycle, and Fn lock (BIOS 0114 and above)
* Change battery charge limit (BIOS 0114 and above)


# How to install
## Downloading
If you have `git` installed:
```
git clone https://github.com/pobrn/qc71_laptop
```

If you don't, then you can download it [here](https://github.com/pobrn/qc71_laptop/archive/master.zip).

## Installing
### Linux headers
On Debian and its [many](https://www.debian.org/derivatives/) [derivatives](https://wiki.ubuntu.com/DerivativeTeam/Derivatives) (Ubuntu, Pop OS, Linux Mint, ...) , run
```
sudo apt install linux-headers-$(uname -r)
```
to install the necessary header files.

On Arch Linux and its derivatives (Manjaro, ...), run
```
sudo pacman -Syu linux-headers
```


### DKMS (optional)
DKMS should be in your distributions repositories. `sudo apt install dkms`, `sudo pacman -Syu dkms` should work depending on your distribution.


### The module
#### Manually
Navigate in a terminal into the directory, then execute `make`. This should compile the module. If everything went correctly, a file named `qc71_laptop.ko` should appear in the directory.

To test the module try `sudo insmod qc71_laptop.ko`. Now you should see the fan speeds appear in the output of `sensors`, and the directory `/sys/devices/platform/qc71_laptop` should now exist. If you are done testing, unload the module using `sudo rmmod qc71_laptop`.

Now you could create a script that inserts this module at boot from this directory, or you could install it using DKMS.

#### With DKMS
Run
```
sudo make dkmsinstall
```
to install the module with DKMS. Or run
```
sudo make dkmsuninstall
```
to uninstall the module.

The module should automatically load at boot after this. If you want to load it immediately, run `sudo modprobe qc71_laptop`. If it reports an error, and you're convinced your device should be supported, please open an issue.


# How to use
## Fan speeds
After loading the module the fan speeds should immediately appear in the output of `sensors`, and all your favourite monitoring utilities (e.g. the [Freon](https://extensions.gnome.org/extension/841/freon/) GNOME shell extension) that use `sensors`.


## Controllig the lightbar
The lightbar is integrated into the LED subsystem of the linux kernel. When the module is loaded, `/sys/class/leds/qc71_laptop::lightbar` directory should exist with the following important files:
```
/sys/class/leds/qc71_laptop::lightbar/brightness
```

It contains `1` if the lightbar is turned on in S0 sleep state (aka. when the device is powered on), `0` otherwise. You can turn on/off the lightbar by writing an appropriate number into this file:
```
# echo 1 > /sys/class/leds/qc71_laptop::lightbar/brightness
```
will turn the lightbar on. (Writing `0` will turn it off.)
To check the current state:
```
$ cat /sys/class/leds/qc71_laptop::lightbar/brightness
```

___
```
/sys/class/leds/qc71_laptop::lightbar/brightness_s3
```
It contains `1` if the lightbar is turned on in S3 sleep state (aka. when the device is sleeping). If it is `1`, the lightbar will "breathe" when the device is sleeping. You can control it the same way as `brightness`.

___
```
/sys/class/leds/qc71_laptop::lightbar/rainbow_mode
```
It contains `1` if the "rainbow mode" is enabled (aka. the color will continuously cycle). Controlling works the same way as before.

*Note:* Enabling/disabling the rainbow mode will not turn the lightbar on/off.
*Note:* The rainbow mode takes precedence over the color.

___
```
/sys/class/leds/qc71_laptop::lightbar/color
```
This file controls the color of the lightbar. It is a three digit number (possibly with padding zeroes). The first digit is the red component, the second one is the green componenet, the third one is the blue component.
```
$ cat /sys/class/leds/qc71_laptop::lightbar/color
```
tells you the current color, while
```
# echo 591 > /sys/class/leds/qc71_laptop::lightbar/color
```
will change the current color.

*Note:* Chaning the color will not turn the lightbar on.


## Controlling the fans
These can be controlled directly from the BIOS as well.

### Passive cooling
I call this feature "always on" because that's less confusing than "passive cooling".
```
# echo 1 > /sys/devices/platform/qc71_laptop/fan_always_on
```
will cause the fans to run continuously. Writing `0` will turn it off.

### Reduced duty cycle
```
# echo 1 > /sys/devices/platform/qc71_laptop/fan_reduced_duty_cycle
```
will cause the fans to run at 25% of their capacity (about 2300 RPM) at idle (instead of 30% - about 2700 RPM). Writing `0` will restore the 30% idle duty cycle.


## Fn lock
```
# echo 1 > /sys/devices/platform/qc71_laptop/fn_lock
```
will disable the Fn lock lock (you read that right), meaning that if you now press Fn+ESC, then the you enable the Fn lock, meaning you won't need to use Fn to access the secondary functions of the buttons in the top row. Pressing Fn+ESC turns the Fn lock off. Writing zero will enable the Fn lock lock. Fn+ESC has no effect if that file contains `0`.


## Battery charge limit
The file `/sys/class/power_supply/BAT0/charge_control_end_threshold` contains the current charge threshold. Writing a number between 1 and 100 will cause the battery charging limit to be set to that percentage. (I did not test extremely low values, so I cannot say if they work). For example:
```
# echo 60 > /sys/class/power_supply/BAT0/charge_control_end_threshold
```
will cause charging to stop when the battery reaches 60% of its capacity.


## Example use

The XMG Control Center can change the color if the device is on battery or plugged in. Fortunately you can easily achieve the same using [acpid](https://wiki.archlinux.org/index.php/Acpid). Modifying the appropriate part of `/etc/acpi/handler.sh` like this:
```
    ac_adapter)
        case "$2" in
            AC|ACAD|ADP0|ACPI0003:00) # the "ACPI0003:00" part was not there by default
                case "$4" in
                    00000000)
                        logger 'AC unplugged'
                        
                        # change color to red
                        echo 900 > /sys/class/leds/qc71_laptop::lightbar/color
                        ;;
                    00000001)
                        logger 'AC plugged'
                        
                        # change color to blue
                        echo 009 > /sys/class/leds/qc71_laptop::lightbar/color
                        ;;
                esac
                ;;
            *)
                logger "ACPI action undefined: $2"
                ;;
        esac
        ;;
```
You can use `acpi_listen` to see what events are generated when you plug the machine in or disconnect the charger. You might need to modify the third line (in this snippet).
