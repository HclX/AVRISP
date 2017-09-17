# AVRISP
This a simple AVR ISP protocol simulation tool.

I was thinking of having an ATTINY working together with ESP8266. One thing I want to make sure is I can easily program the attiny when it's on the final PCB. This means I need to implement the AVR ISP protocol on ESP8266. But before I really put the code into ESP8266, having a simulated version running on desktop maybe much easier for debugging. That results in this project.

The intendend way of using the code is on ESP8266 using arduino framework, so the code was designed with that in mind, even though it's currently running on Win32.
