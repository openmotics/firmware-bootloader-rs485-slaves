import os
import sys
from intelhex import IntelHex

if len(sys.argv) != 4:
    print('Usage: python merge.py <path to application> <path to bootloader> <path to merged>')
    os._exit(1)

application_filename = sys.argv[1]
bootloader_filename = sys.argv[2]
merged_filename = sys.argv[3]

application = IntelHex()
application.loadhex(application_filename)

bootloader = IntelHex()
bootloader.loadhex(bootloader_filename)

merged = IntelHex()
# First, merge the bootloader
merged.merge(bootloader, overlap='replace')
# Then, merge over the application, overwriting reset vector
merged.merge(application, overlap='replace')
# Restore bootloader reset vector
merged[0x0] = bootloader[0x0]
merged[0x1] = bootloader[0x1]
merged[0x2] = bootloader[0x2]
merged[0x3] = bootloader[0x3]
# Put original reset vector at specific location
merged[0xe678] = application[0x0]
merged[0xe679] = application[0x1]
merged[0xe67a] = application[0x2]
merged[0xe67b] = application[0x3]

with open(merged_filename, 'w') as merged_file:
    merged.write_hex_file(merged_file)
