Custom bootloader and board definition file

Version
-------

**v0.1**

Known Issues
------------

Getting Started
---------------

I want to upload Arduino sketches to my BadgerStick (which already has the bootloader).

* Copy boards.txt to <Arduino folder>/hardware/arduino
* Open the Arduino program
* BadgerStick should appear under Tools -> Board

I want to compile the BadgerStick bootloader.

* Copy ATmegaBOOT_168.c and the Makefile located in bootloaders/atmega to <Arduino folder>/hardware/arduino/bootloaders/atmega
* Open a command terminal
* Navigate to <Arduino folder>/hardware/arduino/bootloaders/atmega
* Enter the command: make atmega328_pro4

I want to burn the bootloader onto the BadgerStick.

* If you did not compile the bootloader, copy the ATmegaBOOT_168_atmega328_pro_4MHz.hex file in bootloaders/atmega to <Arduino folder>/hardware/arduino/bootloaders/atmega
* Follow the instructions at [http://arduino.cc/en/Tutorial/ArduinoISP](http://arduino.cc/en/Tutorial/ArduinoISP) to use an Arduino as a programmer for the BadgerStick
* Select "BadgerStick (3.3V, 4MHz)" when instructed to select the board "on which you want to burn the bootloader"
* Use the Tools -> Burn Bootloader option
* Wait for the bootloader to finish burning (you will get a notification in the Arduino program)

What Did You Do?
----------------

To create a custom board for the Arduino program, we added a section under "# Added by SparkFun electronics to support the BadgerStick" in boards.txt.

To create a custom bootloader, we changed a few lines of code in ATmegaBOOT_168.c under "// Fix the EEWE bit definition..." in order to fix a compile error (EEPE not defined). We also changed the OPTIMIZE setting to -Os to reduce the size of the bootloader file. Finally, we added a new Make target under "# Added 4MHz version of the 328P" with new fuse settings to allow for lower voltage and 4MHz operation.

Version History
---------------

**v0.1**

* Initial prototype
* 4MHz with 1.8V brown-out detection Arduino bootloader created for ATmega 328P
* Custom board definition added to boards.txt for the BadgerStick

License
-------

All Arduino code is release under the Creative Commons Attribution Share-Alike license. For more information, see [http://arduino.cc/en/Main/FAQ](http://arduino.cc/en/Main/FAQ).