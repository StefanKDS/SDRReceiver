import RPi.GPIO as GPIO
import alsaaudio
import os
import sys
from subprocess import check_output
import serial
from time import sleep
from pynput.mouse import Controller as MouseController, Button
from pynput.keyboard import Key, Controller as KeyboardController

mouse = MouseController()
keyboard = KeyboardController()
mixer = alsaaudio.Mixer()
current_volume = mixer.getvolume()[0]
ser=serial.Serial('/dev/ttyUSB0', 115200)

y = 70
x1 = 665
x2 = 641
x3 = 617
x4 = 586
x5 = 564
x6 = 544
x7 = 508
x8 = 488
x9 = 467

pos = 4;

print(current_volume)

print("script running")
print (current_volume)

Enc_1_A = 17  
Enc_1_B = 27
Enc_2_A = 24  
Enc_2_B = 23
Button_up = 22
Button_down = 25

 
def init():
    GPIO.setwarnings(True)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(Button_up, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(Button_down, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(Enc_1_A, GPIO.IN)
    GPIO.setup(Enc_1_B, GPIO.IN)
    GPIO.setup(Enc_2_A, GPIO.IN)
    GPIO.setup(Enc_2_B, GPIO.IN)
    GPIO.add_event_detect(Enc_1_A, GPIO.RISING, callback=rotation_decode_frequency, bouncetime=5)
    GPIO.add_event_detect(Enc_2_A, GPIO.RISING, callback=rotation_decode_volume, bouncetime=5)
    GPIO.add_event_detect(Button_up, GPIO.RISING, callback=onButtonUp, bouncetime=50)
    GPIO.add_event_detect(Button_down, GPIO.RISING, callback=onButtonDown, bouncetime=50)
    return

def rotation_decode_frequency(Enc_1_A):
    #win = check_output("xdotool getactivewindow getwindowname", shell=True).decode(sys.stdout.encoding)
    #if( "SDR" in win ):
    rotation_decode_frequency_sdr(Enc_1_A);
    #if( "digi" in win ):
    #    rotation_decode_frequency_digi(Enc_1_A);
    return

def rotation_decode_frequency_sdr(Enc_1_A):
    print("rotation_decode_frequency sdr")
    sleep(0.002)
    Switch_A_1 = GPIO.input(Enc_1_A)
    Switch_B_1 = GPIO.input(Enc_1_B)
 
    if (Switch_A_1 == 1) and (Switch_B_1 == 0):
        mouse.scroll(0,1)
        print("mouse scroll up")
    elif (Switch_A_1 == 1) and (Switch_B_1 == 1):
        mouse.scroll(0,-1)
        print("mouse scroll down")
    return

def rotation_decode_frequency_digi(Enc_1_A):
    print("rotation_decode_frequency digi")
    mouse.position = (400,400)
    sleep(0.002)
    Switch_A_1 = GPIO.input(Enc_1_A)
    Switch_B_1 = GPIO.input(Enc_1_B)
 
    if (Switch_A_1 == 1) and (Switch_B_1 == 0):
        keyboard.press(Key.ctrl)
        keyboard.press(Key.right)
        keyboard.release(Key.right)
        keyboard.release(Key.ctrl)
    elif (Switch_A_1 == 1) and (Switch_B_1 == 1):
        keyboard.press(Key.ctrl)
        keyboard.press(Key.left)
        keyboard.release(Key.left)
        keyboard.release(Key.ctrl)
    return
    
def rotation_decode_volume(Enc_2_A):
    print("rotation_decode_volume")
    global current_volume
    sleep(0.002)
    Switch_A = GPIO.input(Enc_2_A)
    Switch_B = GPIO.input(Enc_2_B)
 
    if (Switch_A == 1) and (Switch_B == 0):
        if (current_volume < 99):
            current_volume += 2
        print("Wheel:",current_volume)
        mixer.setvolume(current_volume)
    elif (Switch_A == 1) and (Switch_B == 1):
        if (current_volume > 1):
            current_volume -= 2
        print("Wheel:",current_volume)
        mixer.setvolume(current_volume)
    return

def getPosCoordinate(position) -> int:
    global pos
    if(pos == 1):
        return x1
    if(pos == 2):
        return x2
    if(pos == 3):
        return x3
    if(pos == 4):
        return x4
    if(pos == 5):
        return x5
    if(pos == 6):
        return x6
    if(pos == 7):
        return x7
    if(pos == 8):
        return x8
    if(pos == 9):
        return x9

def onButtonUp(Button_up):
    global pos
    print("button up")
    print(GPIO.input(Button_up))
    if(GPIO.input(Button_up)):  
        if(pos < 9):
            pos+=1
            mouse.position = (getPosCoordinate(pos),y)
        else:
            pos = 1
            mouse.position = (getPosCoordinate(pos),y)
    return

def onButtonDown(Button_down):
    global pos
    print("button down")
    print(GPIO.input(Button_down))
    if(GPIO.input(Button_down)):
        if(pos > 1):
            pos-=1
            mouse.position = (getPosCoordinate(pos),y)
        else:
            pos = 9
            mouse.position = (getPosCoordinate(pos),y)
        
    return

def onKeyboard():
    global ser
    readedText=ser.readline()
    print(readedText)
    if(readedText == b'B1\r\n'):
        os.system('xdotool search --name "SDR++" | xargs xdotool windowactivate')
    if(readedText == b'B2\r\n'):
        os.system('xdotool search --name "fldigi" | xargs xdotool windowactivate')
    if(readedText == b'B3\r\n'):
        keyboard.press(Key.end)
        sleep(0.05)
        keyboard.release(Key.end)
    if(readedText == b'B4\r\n'):
        keyboard.press(Key.menu)
        sleep(0.05)
        keyboard.release(Key.menu)
    if(readedText == b'B5\r\n'):
        mouse.position = (163,181)
        mouse.press(Button.left)
        mouse.release(Button.left)
    if(readedText == b'B6\r\n'):
        mouse.position = (231,158)
        mouse.press(Button.left)
        mouse.release(Button.left)
    if(readedText == b'B7\r\n'):
        mouse.position = (95,158)
        mouse.press(Button.left)
        mouse.release(Button.left)
    if(readedText == b'B9\r\n'):
        os.system("systemctl poweroff")
    return
 
def main():
    try:
        init()
        while True :
            onKeyboard()
            sleep(0.1)
 
    except KeyboardInterrupt:
        GPIO.cleanup()
 
if __name__ == '__main__':
    main()