# Written by Brian Ejike
# Distributed under the MIT License

import serial, time

WIDTH = 256
HEIGHT = 288
DEPTH = 8

HALF_BITMAP_SIZE = int(WIDTH * HEIGHT / 2)

portSettings = ['', 0]

print("----------Extract Fingerprint Image------------")
print()

def assembleHeader(width, height, depth, cTable=False):
    header = bytearray(54)
    header[0:2] = b'BM'   # bmp signature
    byte_width = int((depth*width + 31) / 32) + 4
    if cTable:
        header[2:6] = ((byte_width * height) + (2**depth)*4 + 54).to_bytes(4, byteorder='little')  #file size
    else:
        header[2:6] = ((byte_width * height) + 54).to_bytes(4, byteorder='little')  #file size
    #header[6:10] = (0).to_bytes(4, byteorder='little')
    if cTable:
        header[10:14] = ((2**depth) * 4 + 54).to_bytes(4, byteorder='little') #offset
    else:
        header[10:14] = (54).to_bytes(4, byteorder='little') #offset

    header[14:18] = (40).to_bytes(4, byteorder='little')    #header size
    header[18:22] = width.to_bytes(4, byteorder='little') #width
    header[22:26] = height.to_bytes(4, byteorder='little') #height
    header[26:28] = (1).to_bytes(2, byteorder='little') #no of planes
    header[28:30] = depth.to_bytes(2, byteorder='little') #depth
    #header[30:34] = (0).to_bytes(4, byteorder='little')
    header[34:38] = (byte_width * height).to_bytes(4, byteorder='little') #image size
    header[38:42] = (1).to_bytes(4, byteorder='little') #resolution
    header[42:46] = (1).to_bytes(4, byteorder='little')
    #header[46:50] = (0).to_bytes(4, byteorder='little')
    #header[50:54] = (0).to_bytes(4, byteorder='little')
    return header

def options():
    print("Options:")
    print("\tPress 1 to enter serial port settings")
    print("\tPress 2 to scan a fingerprint and save the image")
    print("\tPress 3 to view help")
    print("\tPress 4 to exit")
    print()
    choice = input(">> ")
    print()
    return choice

def getSettings():
    portSettings[0] = input("Enter Arduino Serial port number: ")
    portSettings[1] = int(input('Enter Serial baud rate: '))
    print()
    
def getPrint():
    '''
    First enter the port settings with menu option 1:
    >>> Enter Arduino Serial port number: COM13
    >>> Enter Serial baud rate: 57600

    Then enter the filename of the image with menu option 2: 
    >>> Enter filename/path of output file (without extension): myprints
    Found fingerprint sensor!
    .
    .
    .
    (Here you communicate with the Arduino and follow instructions)
    .
    .
    .
    Extracting image...saved as <filename>.bmp

    '''
    out = open(input("Enter filename/path of output file (without extension): ")+'.bmp', 'wb')
    out.write(assembleHeader(WIDTH, HEIGHT, DEPTH, True))
    for i in range(256):   # set up palette
        out.write(i.to_bytes(1,byteorder='little')*4)
    try:
        ser = serial.Serial(portSettings[0], portSettings[1], timeout=1)
    except:
        print('Invalid port settings!')
        print()
        out.close()
        return
    while ser.isOpen():
        try:
            curr = str(ser.read(), encoding='utf-8')
            if curr == '\t':
                print('Extracting image...', end='')
                for i in range(HALF_BITMAP_SIZE):
                    byte = ser.read()
                    if not byte:
                        print("Timeout!")
                        out.close()
                        ser.close()
                        return False
                    out.write((byte[0] & 0xf0).to_bytes(1, byteorder='little') * 2)
                out.close()
                ser.close()
                print('saved as', out.name)
                print()
                return True
            else:
                print(curr, end='')
        except Exception as e:
            print(e)
            out.close()
            ser.close()
            return False

while True:
    chose = options()
    if chose == "4":
        break
    elif chose == '1':
        getSettings()
    elif chose == "2":
        res = getPrint()
        if not res:
            print("Image extraction failed!")
        continue
    elif chose == '3':
        print('================= HELP ==================')
        print(getPrint.__doc__)
        print('=========================================')
