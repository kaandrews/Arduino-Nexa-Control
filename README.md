# Arduino-Nexa-Control
Experiments with controlling Nexa 433 MHz remote control sockets using Arduino

For these experiments I used some cheap transmitters and receivers from Amazon:
https://www.amazon.co.uk/gp/product/B00R2U8OEU/

The Nexa Control Serial Arduino sketch allows you to send codes to turn on and off the sockets
The sockets need to be programmed to match the transmitter code, so there are 2 options:
1) Program a new code using an ID you choose
2) Copy the code from your current transmitter (see the Nexa RX Serial sketch)

The Nexa RX Serial sketch allows you to use a receive and decode the transmissions from a Nexa transmitter
You can use this to work out correct code to use for the transmitter, or maybe to control something connected to the Arduino using a Nexa remote.

The next phase of my project is to integrate the transmitter into Samsung Smartthings to automate control

Let me know if you find any of it useful!
