# SDRReceiver

I build my own SDR-Receiver based on a RaspberryPi 4 with 7"-Display, RTL-SDR V3 Stick, SecondDisplay with ESP 8266 and two Rotary Encoders for volume and frequency selection.
The SDR Software is SDRPlusPlus with a selfmade plugin "Logbook" to log stations I heard.

A more detailed description will follow in the next days....

# The components  
## Raspberry Pi
It's a Raspberry Pi 4 with 4GB RAM located in a housing with 7" touchdisplay.
The OS is a standard Raspberry Pi OS with [SDRPlusPlus](https://github.com/AlexandreRouma/SDRPlusPlus) installed.
For the interaction with the RotaryEncoders and the second display I use the python script 
[here](https://github.com/StefanKDS/SDRReceiver/blob/main/Controller/rotary1.py).

## SDRPlusPlus
As receiving software I use the standard SDRPlusPlus.
I wrote a [custom plugin](https://github.com/StefanKDS/SDRReceiver/tree/main/SDRPlusPlus_Plugins/logbook) to log stations I heard.  

## SDR
As the receiver itself I use the [RTL-SDR V3](https://www.rtl-sdr.com/).

## RotaryEncoder
The inpout of the RotaryEncoders will pe processed by the python script [here](https://github.com/StefanKDS/SDRReceiver/blob/main/Controller/rotary1.py).

RotaryEncoder 1 is used to control the volume of the audio output. 

RotaryEncoder 2 is used to select the frequency in SDRPlusPlus. With a button press on the RotaryEncoder 2 you can select the digit and with turning the 
Encoder you can go up and down with the freqeuncy.

## Second Display
.... to be continue...

# Some impressions

![plot](https://github.com/StefanKDS/SDRReceiver/blob/main/Pictures/IMG20231119083627.jpg?raw=true)

![plot](https://github.com/StefanKDS/SDRReceiver/blob/main/Pictures/IMG20231119083752.jpg?raw=true)

![plot](https://github.com/StefanKDS/SDRReceiver/blob/main/Pictures/IMG20231119083804.jpg?raw=true)

![plot](https://github.com/StefanKDS/SDRReceiver/blob/main/Pictures/IMG20231119083821.jpg?raw=true)
