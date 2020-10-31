Copyright (c) 2020 Damien Le Moal.

# Linux 6-DoF Arm Demonstration

This simple program illustrates how to control servos with a Linux application,
using a PCA9685 16-channels I2C PWM controller. The application demonstrated
here controls a 6 DoF robotic arm. The servos iused are MG996R.

## Compilation

For native compilation, make sure the *ncurses* library and its development
headers are installed. Then simply execute `make` to compile the program.

Cross compilation is defined for RISC-V no MMU environment. To cross compile,
run:

```
> make CROSS_COMPILE=riscv64-buildroot-linux-uclibc-
  CC main.c
  CC pca9685.c
  CC servo.c
Building demo
```

## Execution

Simply run:

```
> sudo ./demo /dev/i2c-X
```

Where /dev/i2c-X is the I2C device file name of the master I2C device to which
the PCA9685 controller is attached. To find out which device file to use, the
`i2cdetect` command can be used.

```
i2cdetect -l
i2c-3	i2c       	i915 gmbus dpd                  	I2C adapter
i2c-1	i2c       	i915 gmbus dpc                  	I2C adapter
i2c-6	i2c       	AUX A/port E                    	I2C adapter
i2c-4	i2c       	CP2112 SMBus Bridge on hidraw0  	I2C adapter
i2c-2	i2c       	i915 gmbus misc                 	I2C adapter
i2c-0	i2c       	i915 gmbus dpb                  	I2C adapter
i2c-5	i2c       	AUX B/port B                    	I2C adapter
```

In the above example, the I2C master used is a CP2112 chip based USB-to-I2C
bridge, represented as /dev/i2c-4. The `demo` application can list I2C adapters
using the -l option.

```
> ./demo -h
Usage: ./demo [options] <i2c dev>
Options:
    -h: help
    -l: list i2c adapters
    -a <addr (HEX)>: i2c slave address
```

The `demo` application has an internal menu to control operations.

```
./demo /dev/i2c-4
Connecting i2c servo controller at 0x40, 50 Hz on /dev/i2c-4...
Resetting controller...
Moving to initial position...
Command ('h' for help): h

Commands:
    h: Help
    i: Interactive mode. Type servo number
       (0 to 5), then arrow up/down to move
    r: Reset to initial position
    d: Run demonstration
    q: Quit
Command ('h' for help):
```

## Example

[This video](https://damien-lemoal.github.io/linux-robot-arm/#example) shows a
6-DoF robot arm in action with a tiny SiPeed K210 RISC-V board used as a host.

<video width="240" controls>
  <source type="video/mp4" src="example/riscv-arm.mp4">
</video>

The Linux kernel tree for this board is available
[here](https://github.com/damien-lemoal/linux), in the branch *k210-sysctl-v12*.
The root FS was built using
[this buildroot tree](https://github.com/damien-lemoal/buildroot).
