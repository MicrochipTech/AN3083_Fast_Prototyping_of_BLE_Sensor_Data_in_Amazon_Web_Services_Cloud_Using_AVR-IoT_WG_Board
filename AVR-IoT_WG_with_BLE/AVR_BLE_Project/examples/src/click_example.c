/**
 * \file
 *
 *
 (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms,you may use this software and
    any derivatives exclusively with Microchip products.It is your responsibility
    to comply with third party license terms applicable to your use of third party
    software (including open source software) that may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 */

#include <stdio.h>
#include <string.h>
#include <click_example.h>
#include <clock_config.h>
#include <util/delay.h>
#include "rn4871_click.h"
#include "driver_rn4871.h"
#include "usart_basic.h"

#define CONNECTED 0x01
#define DISCONNECTED 0x00

uint8_t checkStatus(const char *status)
{
	uint8_t ret;
	_delay_ms(1000);
	ret = RN4871_CheckResponse(status);
	RN4871_ClearReceivedMessage();
	return ret;
}

/**
  Section: RN4871 Click Example Code
 */

void RN4871_Example()
{
	uint8_t        connected = 0, firstTime = 1;
	char           Name[9]            = "Spartans";
	static uint8_t RN4871_Initialized = 0;

	if (!RN4871_Initialized) {
		rn4871_InitLED_Set(true);
		RN4871_Setup(Name);
		RN4871_Initialized = 1;
		rn4871_InitLED_Set(false);
	}

	connected = checkStatus("%CONNECT");
	while (connected) {
		if (RN4871_CheckResponse("%DISCONNECT")) {
			connected = 0;
			RN4871_ClearReceivedMessage();
			break;
		}
		if (firstTime) {
			RN4871_sendAndWait("$$$", "CMD> ", 40);
			firstTime = 0;
		}

		_delay_ms(500);
		rn4871_SendString("SHW,0072,19\r\n");
		_delay_ms(500);
		rn4871_SendString("SHW,0072,32\r\n");
		_delay_ms(500);
		rn4871_SendString("SHW,0072,4B\r\n");
		_delay_ms(500);
		rn4871_SendString("SHW,0072,64\r\n");
	}
}
