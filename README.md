# RPi-3-VVVF
Making a VVVF inverter with a Raspberry Pi 3.
It does not contain sample for generating vvvf sound.
You have a way to generate the code using vvvf-simulator.

# terms of use
## disclaimer
YOU ARE RESPONSIBLE FOR ANYTHING THAT HAPPENS IF YOU USE THE INFORMATION FROM THIS PROJECT.<br>
IT MAY DAMAGE ELECTRONICS.<br>
THIS VVVF CODE IS NOT MADE BY AN ENGINEER OR ANY PROFESSIONAL.<br>

## credit
The main author of this program is VvvfGeeks except for the code with assembler.<br>
Please reference this github url if you upload on youtube or other websites.

# references
[Peripheral Specifics](https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf)<br>
[Multicore code reference](https://github.com/LdB-ECM/Raspberry-Pi-Multicore/tree/master) (I borrowed some code from here.)

# requirements
You need to install cross compiler.<br>
```
sudo apt install gcc-arm-none-eabi
```

# build
Normally, just run <br>
`make Pi3`

# install to RPi 3
When you have built the code, you will find `kernel7.img` inside of build folder.<br>
What you need to do is

## Download the necessary files
From https://github.com/raspberrypi/firmware/tree/master/boot , you have to get <br>
 - bootcode.bin
 - start.elf
 - fixup.dat

## SD card
Now, you need to have a SD card which has more than 2GB but less than 32GB.<br>
Also the SD card needs to be formated with FAT32.<br>
<br>

## Install
Now , it's time to install.<br>
Copy `bootcode.bin` , `start.elf`, ' fixup.dat' and `kernel7.img` that you have built.<br>


# VVVF pin out
This number is the BCM GPIO number.
- PIN_U_HIGH_2 12
- PIN_U_HIGH_1 13
- PIN_U_LOW_1 11
- PIN_U_LOW_2 21

- PIN_V_HIGH_2 16
- PIN_V_HIGH_1 6
- PIN_V_LOW_1 9
- PIN_V_LOW_2 26

- PIN_W_HIGH_2 20
- PIN_W_HIGH_1 5
- PIN_W_LOW_1 10
- PIN_W_LOW_2 19

- debug_PIN_2 23
- debug_PIN 24

# Function pin out
This number is the BCM GPIO number

## Mascon (Speed controller)
### pin out
 - mascon_1 4
 - mascon_2 17
 - mascon_3 27
 - mascon_4 22

Inside the program, this will generate a integer by using mascon_1 ~ mason_4.<br>
This is the how the integer will be.
`mascon_status_value = input(mascon_1) | input(mascon_2)<<1 | input(mascon_3)<<2 | input(mascon_4)<<3`<br>

When mascon_status_value equals to 4 , the motor will remain at the same speed.
If it is less than 4 (3,2,1,0), vvvf frequency will decrease. 0 is most strong frequency decrease.
If it is more than 4 (5,6,7,8), vvvf frequency will increase. 8 is most strong frequency increase.

## Control button
### pin out
 - button_R 7
 - button_SEL 8
 - button_L 18
