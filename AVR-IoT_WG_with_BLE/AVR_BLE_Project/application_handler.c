/*
    \file   sensors_handling.c

    \brief  Sensors handling handler source file.

    (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip software and any
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party
    license terms applicable to your use of third party software (including open source software) that
    may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
*/

#include <atmel_start.h>
#include <string.h>
#include "sensors_handling.h"
#include "application_handler.h"
#include "driver_rn4871.h"
#include "usart_basic.h"
#include "rn4871_click.h"

#define MAX_SENSORS_BUFFER_SIZE                (MAX_LOCATION + 1)
#define END_OF_CMD                              ('\n')

void application_initialize(void);
void application_updateStateMachine(void);

static void Timer_start(void);
static void Timer_stop(void);
static void updateCharacteristicsValue(void);
static void getSensorsValues(void);

static void commandStateHandler(void);
static void applicationStateHandler(void);

extern void dataTransfer_updateCharacteristicsHandler(uint8_t newData);
extern void dataTransfer_replyToTerminal(const char *reply, uint8_t replyLength);
extern void dataTransfer_updateCharacteristicsValues(uint8_t characteristicsType, char sensorValue[4]);
extern void dataTransfer_RN4871ToTerminal(void);
extern void dataTransfer_terminalToRN4871(void);

extern void rn4871_clearReceivedMessage(void);

static const char commandStateReply[] = {"\r\nCommand State Activated\r\n"};
static const char applicationStateReply[] = {"\r\nApplication State Activated\r\n"};

uint8_t applicationState = APPLICATION_STATE;

enum applicationEvents applicationEvent = NO_EVENT;

volatile uint8_t lightSensorValue[MAX_SENSORS_BUFFER_SIZE];
volatile uint8_t tempSensorValue[MAX_SENSORS_BUFFER_SIZE];

extern uint8_t dataFromTerminal;
extern volatile uint8_t  RN4871_dataBuffer[];

void application_initialize(void)
{
    USART_TERMINAL_set_ISR_cb(dataTransfer_terminalToRN4871, RX_CB);
    
    Timer_start();
    
    applicationEvent = NO_EVENT;
}

void application_updateStateMachine(void)
{
    switch (applicationState)
    {
        case COMMAND_STATE:
            commandStateHandler();
            break;
        
        case APPLICATION_STATE:
            applicationStateHandler();
            break;
        
        default:
            break;
    }
    
    applicationEvent = NO_EVENT;
}

static void commandStateHandler(void)
{
    switch (applicationEvent)
    {
        case DATA_FROM_TERMINAL_EVENT:
            rn4871_SendByte(dataFromTerminal);
            if (dataFromTerminal == END_OF_CMD)
            {
                rn4871_clearReceivedMessage();
                RN4871_blockingWait(2);
                dataTransfer_replyToTerminal(RN4871_dataBuffer, strlen(RN4871_dataBuffer));
            }

            break;
        
        case TOGGLE_EVENT:
            applicationState = APPLICATION_STATE;
            dataTransfer_replyToTerminal(applicationStateReply, strlen(applicationStateReply));
            Timer_start();
            break;
        
        default:
            break;
    }    
}

static void applicationStateHandler(void)
{
    switch (applicationEvent)
    {
        case TIME_TO_TRANSMIT_EVENT:
            getSensorsValues();
            updateCharacteristicsValue();
            break;
        
        case TOGGLE_EVENT:
            applicationState = COMMAND_STATE;
            Timer_stop();
            dataTransfer_replyToTerminal(commandStateReply, strlen(commandStateReply));
            break;
        
        default:
            break;
    }
}

static void getSensorsValues(void)
{
    volatile uint16_t lightValue = 0;
    volatile int16_t tempValue = 0;
    
    lightValue = SENSORS_getLightValue();
    sprintf(lightSensorValue, "%.4x", lightValue);  
    
    tempValue = SENSORS_getTempValue() / 100;
    sprintf(tempSensorValue, "%.4x", tempValue);      
}

static void updateCharacteristicsValue(void)
{
    dataTransfer_updateCharacteristicsValues(LIGHT_COMMAND_HANDLER, lightSensorValue);
    dataTransfer_updateCharacteristicsValues(TEMP_COMMAND_HANDLER, tempSensorValue);   
}

static void Timer_start(void)
{
    // set the starting count value to 0 and enable the module
    TCA0.SINGLE.CNT = 0x0;
    TCA0.SINGLE.CTRLA |= 1 << TCA_SINGLE_ENABLE_bp;
}

static void Timer_stop(void)
{
    TCA0.SINGLE.CTRLA &= ~(1 << TCA_SINGLE_ENABLE_bp);
}