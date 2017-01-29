

More info to be found on the wiki:
http://wiki.openmotics.com/index.php/Bootloader

There are 2 versions: one for 18f45K80 and one for 18f46K80 with RS485 communication.
Then use the bat file to generate the combination version of the bootloader and the program itself. Otherwise you have a chicken egg problem. The Bootloader needs an address but that can only be set by the program. 
The Checksum will automatically be calculated the first time the program is started and the address type is set to 0xFF.
 
A bootloader version is made for every type of module because the leds and buttons are not assigned to the same pins. This can be set in ModuleType.h



Some memo.
----------

BL    :0E 0000 00 1D 12 7F 20 008991000FC00FE00F40FD
PROG  :0E 0000 00 15 08 7F 27 008991000FC00FE00F4008
 
300000 1D 15
300001 12 08

300003 20 27 
 
set in osccon en osctune
 
 
jumpaddr
:105FF000FFFFFFFFFFFFFFFFE8EF00F0FFFFFFFFE6

start
:10000000BBEF3DF0FFFFFFFF70EF03F0FFFFFFFFCF