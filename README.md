(AN3083) Fast Prototyping of BLE Sensor Data in Amazon Web Services Cloud Using AVR-IoT WG Board
===
http://ww1.microchip.com/downloads/en/AppNotes/Fast-Prototyping-of-BLE-Sensors-in-AWS-Cloud-Using-AVR-IoT-Development-Boards-00003083A.pdf?fbclid=IwAR1-7oaOAMQtBhnmjDPGQoIkzx6CSVN5pqVLGLqTXRRiWdD0rXv4HswMmEg

This repository contains the project source code for the AVR-IoT WG Development Board 
together with the Lambda used by the gateway to read characteristics and publish 
the data to the Amazon Web Services cloud.

The repository contains an Atmel Studio Solution. The project is designed for 
the AVR-IoT WG Development Board which includes an ATmega4808 microcontroller 
and a temperature and light sensor.

Make sure you have one of the necessary Integrated Development Environments (IDE):

	1. Atmel Studio v7 (used in this project) with the latest device packages installed
	2. MPLAB® X v5.15 with XC8 v2.05 compiler


Make sure you have the necessary hardware:

	1. Microchip AVR-IoT WG Development Board
	2. Bluetooth Low Energy RN4871 Click board from MIKROE
	3. Raspberry Pi 3 Model B+