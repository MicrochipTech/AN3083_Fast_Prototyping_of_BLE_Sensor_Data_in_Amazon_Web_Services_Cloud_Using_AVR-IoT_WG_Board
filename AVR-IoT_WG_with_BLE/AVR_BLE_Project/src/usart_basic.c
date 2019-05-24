/**
 * \file
 *
 * \brief USART basic driver.
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

/**
 * \defgroup doc_driver_usart_basic USART Basic
 * \ingroup doc_driver_usart
 *
 * \section doc_driver_usart_basic_rev Revision History
 * - v0.0.0.1 Initial Commit
 *
 *@{
 */
#include <compiler.h>
#include <clock_config.h>
#include <usart_basic.h>
#include <atomic.h>

#include <stdio.h>

#if defined(__GNUC__)

int USART_RN4871_printCHAR(char character, FILE *stream)
{
	USART_RN4871_write(character);
	return 0;
}

FILE USART_RN4871_stream = FDEV_SETUP_STREAM(USART_RN4871_printCHAR, NULL, _FDEV_SETUP_WRITE);

#elif defined(__ICCAVR__)

int putchar(int outChar)
{
	USART_0_write(outChar);
	return outChar;
}
#endif

/* Static Variables holding the ringbuffer used in IRQ mode */
static uint8_t          USART_RN4871_rxbuf[USART_RN4871_RX_BUFFER_SIZE];
static volatile uint8_t USART_RN4871_rx_head;
static volatile uint8_t USART_RN4871_rx_tail;
static volatile uint8_t USART_RN4871_rx_elements;
static uint8_t          USART_RN4871_txbuf[USART_RN4871_TX_BUFFER_SIZE];
static volatile uint8_t USART_RN4871_tx_head;
static volatile uint8_t USART_RN4871_tx_tail;
static volatile uint8_t USART_RN4871_tx_elements;

void USART_RN4871_default_rx_isr_cb(void);
void (*USART_RN4871_rx_isr_cb)(void) = &USART_RN4871_default_rx_isr_cb;
void USART_RN4871_default_udre_isr_cb(void);
void (*USART_RN4871_udre_isr_cb)(void) = &USART_RN4871_default_udre_isr_cb;

void USART_RN4871_default_rx_isr_cb(void)
{
	uint8_t data;
	uint8_t tmphead;

	/* Read the received data */
	data = USART1.RXDATAL;
	/* Calculate buffer index */
	tmphead = (USART_RN4871_rx_head + 1) & USART_RN4871_RX_BUFFER_MASK;

	if (tmphead == USART_RN4871_rx_tail) {
		/* ERROR! Receive buffer overflow */
	} else {
		/* Store new index */
		USART_RN4871_rx_head = tmphead;

		/* Store received data in buffer */
		USART_RN4871_rxbuf[tmphead] = data;
		USART_RN4871_rx_elements++;
	}
}

void USART_RN4871_default_udre_isr_cb(void)
{
	uint8_t tmptail;

	/* Check if all data is transmitted */
	if (USART_RN4871_tx_elements != 0) {
		/* Calculate buffer index */
		tmptail = (USART_RN4871_tx_tail + 1) & USART_RN4871_TX_BUFFER_MASK;
		/* Store new index */
		USART_RN4871_tx_tail = tmptail;
		/* Start transmission */
		USART1.TXDATAL = USART_RN4871_txbuf[tmptail];
		USART_RN4871_tx_elements--;
	}

	if (USART_RN4871_tx_elements == 0) {
		/* Disable UDRE interrupt */
		USART1.CTRLA &= ~(1 << USART_DREIE_bp);
	}
}

/**
 * \brief Set call back function for USART_RN4871
 *
 * \param[in] cb The call back function to set
 *
 * \param[in] type The type of ISR to be set
 *
 * \return Nothing
 */
void USART_RN4871_set_ISR_cb(usart_cb_t cb, usart_cb_type_t type)
{
	switch (type) {
	case RX_CB:
		USART_RN4871_rx_isr_cb = cb;
		break;
	case UDRE_CB:
		USART_RN4871_udre_isr_cb = cb;
		break;
	default:
		// do nothing
		break;
	}
}

/* Interrupt service routine for RX complete */
ISR(USART1_RXC_vect)
{
	if (USART_RN4871_rx_isr_cb != NULL)
		(*USART_RN4871_rx_isr_cb)();
}

/* Interrupt service routine for Data Register Empty */
ISR(USART1_DRE_vect)
{
	if (USART_RN4871_udre_isr_cb != NULL)
		(*USART_RN4871_udre_isr_cb)();
}

bool USART_RN4871_is_tx_ready()
{
	return (USART_RN4871_tx_elements != USART_RN4871_TX_BUFFER_SIZE);
}

bool USART_RN4871_is_rx_ready()
{
	return (USART_RN4871_rx_elements != 0);
}

bool USART_RN4871_is_tx_busy()
{
	return (!(USART1.STATUS & USART_TXCIF_bm));
}

/**
 * \brief Read one character from USART_RN4871
 *
 * Function will block if a character is not available.
 *
 * \return Data read from the USART_RN4871 module
 */
uint8_t USART_RN4871_read(void)
{
	uint8_t tmptail;

	/* Wait for incoming data */
	while (USART_RN4871_rx_elements == 0)
		;
	/* Calculate buffer index */
	tmptail = (USART_RN4871_rx_tail + 1) & USART_RN4871_RX_BUFFER_MASK;
	/* Store new index */
	USART_RN4871_rx_tail = tmptail;
	ENTER_CRITICAL(R);
	USART_RN4871_rx_elements--;
	EXIT_CRITICAL(R);

	/* Return data */
	return USART_RN4871_rxbuf[tmptail];
}

/**
 * \brief Write one character to USART_RN4871
 *
 * Function will block until a character can be accepted.
 *
 * \param[in] data The character to write to the USART
 *
 * \return Nothing
 */
void USART_RN4871_write(const uint8_t data)
{
	uint8_t tmphead;

	/* Calculate buffer index */
	tmphead = (USART_RN4871_tx_head + 1) & USART_RN4871_TX_BUFFER_MASK;
	/* Wait for free space in buffer */
	while (USART_RN4871_tx_elements == USART_RN4871_TX_BUFFER_SIZE)
		;
	/* Store data in buffer */
	USART_RN4871_txbuf[tmphead] = data;
	/* Store new index */
	USART_RN4871_tx_head = tmphead;
	ENTER_CRITICAL(W);
	USART_RN4871_tx_elements++;
	EXIT_CRITICAL(W);
	/* Enable UDRE interrupt */
	USART1.CTRLA |= (1 << USART_DREIE_bp);
}

/**
 * \brief Initialize USART interface
 * If module is configured to disabled state, the clock to the USART is disabled
 * if this is supported by the device's clock system.
 *
 * \return Initialization status.
 * \retval 0 the USART init was successful
 * \retval 1 the USART init was not successful
 */
int8_t USART_RN4871_init()
{

	USART1.BAUD = (uint16_t)USART1_BAUD_RATE(115200); /* set baud rate register */

	USART1.CTRLA = 0 << USART_ABEIE_bp    /* Auto-baud Error Interrupt Enable: disabled */
	               | 0 << USART_DREIE_bp  /* Data Register Empty Interrupt Enable: disabled */
	               | 0 << USART_LBME_bp   /* Loop-back Mode Enable: disabled */
	               | USART_RS485_OFF_gc   /* RS485 Mode disabled */
	               | 1 << USART_RXCIE_bp  /* Receive Complete Interrupt Enable: enabled */
	               | 0 << USART_RXSIE_bp  /* Receiver Start Frame Interrupt Enable: disabled */
	               | 0 << USART_TXCIE_bp; /* Transmit Complete Interrupt Enable: disabled */

	USART1.CTRLB = 0 << USART_MPCM_bp       /* Multi-processor Communication Mode: disabled */
	               | 0 << USART_ODME_bp     /* Open Drain Mode Enable: disabled */
	               | 1 << USART_RXEN_bp     /* Reciever enable: enabled */
	               | USART_RXMODE_NORMAL_gc /* Normal mode */
	               | 0 << USART_SFDEN_bp    /* Start Frame Detection Enable: disabled */
	               | 1 << USART_TXEN_bp;    /* Transmitter Enable: enabled */

	// USART1.CTRLC = USART_CMODE_ASYNCHRONOUS_gc /* Asynchronous Mode */
	//		 | USART_CHSIZE_8BIT_gc /* Character size: 8 bit */
	//		 | USART_PMODE_DISABLED_gc /* No Parity */
	//		 | USART_SBMODE_1BIT_gc; /* 1 stop bit */

	USART1.DBGCTRL = 1 << USART_DBGRUN_bp; /* Debug Run: enabled */

	// USART1.EVCTRL = 0 << USART_IREI_bp; /* IrDA Event Input Enable: disabled */

	// USART1.RXPLCTRL = 0x0 << USART_RXPL_gp; /* Receiver Pulse Length: 0x0 */

	// USART1.TXPLCTRL = 0x0 << USART_TXPL_gp; /* Transmit pulse length: 0x0 */

	uint8_t x;

	/* Initialize ringbuffers */
	x = 0;

	USART_RN4871_rx_tail     = x;
	USART_RN4871_rx_head     = x;
	USART_RN4871_rx_elements = x;
	USART_RN4871_tx_tail     = x;
	USART_RN4871_tx_head     = x;
	USART_RN4871_tx_elements = x;

#if defined(__GNUC__)
	stdout = &USART_RN4871_stream;
#endif

	return 0;
}

/**
 * \brief Enable RX and TX in USART_RN4871
 * 1. If supported by the clock system, enables the clock to the USART
 * 2. Enables the USART module by setting the RX and TX enable-bits in the USART control register
 *
 * \return Nothing
 */
void USART_RN4871_enable()
{
	USART1.CTRLB |= USART_RXEN_bm | USART_TXEN_bm;
}

/**
 * \brief Enable RX in USART_RN4871
 * 1. If supported by the clock system, enables the clock to the USART
 * 2. Enables the USART module by setting the RX enable-bit in the USART control register
 *
 * \return Nothing
 */
void USART_RN4871_enable_rx()
{
	USART1.CTRLB |= USART_RXEN_bm;
}

/**
 * \brief Enable TX in USART_RN4871
 * 1. If supported by the clock system, enables the clock to the USART
 * 2. Enables the USART module by setting the TX enable-bit in the USART control register
 *
 * \return Nothing
 */
void USART_RN4871_enable_tx()
{
	USART1.CTRLB |= USART_TXEN_bm;
}

/**
 * \brief Disable USART_RN4871
 * 1. Disables the USART module by clearing the enable-bit(s) in the USART control register
 * 2. If supported by the clock system, disables the clock to the USART
 *
 * \return Nothing
 */
void USART_RN4871_disable()
{
	USART1.CTRLB &= ~(USART_RXEN_bm | USART_TXEN_bm);
}

/**
 * \brief Get recieved data from USART_RN4871
 *
 * \return Data register from USART_RN4871 module
 */
uint8_t USART_RN4871_get_data()
{
	return USART1.RXDATAL;
}

/* Static Variables holding the ringbuffer used in IRQ mode */
static uint8_t          USART_TERMINAL_rxbuf[USART_TERMINAL_RX_BUFFER_SIZE];
static volatile uint8_t USART_TERMINAL_rx_head;
static volatile uint8_t USART_TERMINAL_rx_tail;
static volatile uint8_t USART_TERMINAL_rx_elements;
static uint8_t          USART_TERMINAL_txbuf[USART_TERMINAL_TX_BUFFER_SIZE];
static volatile uint8_t USART_TERMINAL_tx_head;
static volatile uint8_t USART_TERMINAL_tx_tail;
static volatile uint8_t USART_TERMINAL_tx_elements;

void USART_TERMINAL_default_rx_isr_cb(void);
void (*USART_TERMINAL_rx_isr_cb)(void) = &USART_TERMINAL_default_rx_isr_cb;
void USART_TERMINAL_default_udre_isr_cb(void);
void (*USART_TERMINAL_udre_isr_cb)(void) = &USART_TERMINAL_default_udre_isr_cb;

void USART_TERMINAL_default_rx_isr_cb(void)
{
	uint8_t data;
	uint8_t tmphead;

	/* Read the received data */
	data = USART2.RXDATAL;
	/* Calculate buffer index */
	tmphead = (USART_TERMINAL_rx_head + 1) & USART_TERMINAL_RX_BUFFER_MASK;

	if (tmphead == USART_TERMINAL_rx_tail) {
		/* ERROR! Receive buffer overflow */
	} else {
		/* Store new index */
		USART_TERMINAL_rx_head = tmphead;

		/* Store received data in buffer */
		USART_TERMINAL_rxbuf[tmphead] = data;
		USART_TERMINAL_rx_elements++;
	}
}

void USART_TERMINAL_default_udre_isr_cb(void)
{
	uint8_t tmptail;

	/* Check if all data is transmitted */
	if (USART_TERMINAL_tx_elements != 0) {
		/* Calculate buffer index */
		tmptail = (USART_TERMINAL_tx_tail + 1) & USART_TERMINAL_TX_BUFFER_MASK;
		/* Store new index */
		USART_TERMINAL_tx_tail = tmptail;
		/* Start transmission */
		USART2.TXDATAL = USART_TERMINAL_txbuf[tmptail];
		USART_TERMINAL_tx_elements--;
	}

	if (USART_TERMINAL_tx_elements == 0) {
		/* Disable UDRE interrupt */
		USART2.CTRLA &= ~(1 << USART_DREIE_bp);
	}
}

/**
 * \brief Set call back function for USART_TERMINAL
 *
 * \param[in] cb The call back function to set
 *
 * \param[in] type The type of ISR to be set
 *
 * \return Nothing
 */
void USART_TERMINAL_set_ISR_cb(usart_cb_t cb, usart_cb_type_t type)
{
	switch (type) {
	case RX_CB:
		USART_TERMINAL_rx_isr_cb = cb;
		break;
	case UDRE_CB:
		USART_TERMINAL_udre_isr_cb = cb;
		break;
	default:
		// do nothing
		break;
	}
}

/* Interrupt service routine for RX complete */
ISR(USART2_RXC_vect)
{
	if (USART_TERMINAL_rx_isr_cb != NULL)
		(*USART_TERMINAL_rx_isr_cb)();
}

/* Interrupt service routine for Data Register Empty */
ISR(USART2_DRE_vect)
{
	if (USART_TERMINAL_udre_isr_cb != NULL)
		(*USART_TERMINAL_udre_isr_cb)();
}

bool USART_TERMINAL_is_tx_ready()
{
	return (USART_TERMINAL_tx_elements != USART_TERMINAL_TX_BUFFER_SIZE);
}

bool USART_TERMINAL_is_rx_ready()
{
	return (USART_TERMINAL_rx_elements != 0);
}

bool USART_TERMINAL_is_tx_busy()
{
	return (!(USART2.STATUS & USART_TXCIF_bm));
}

/**
 * \brief Read one character from USART_TERMINAL
 *
 * Function will block if a character is not available.
 *
 * \return Data read from the USART_TERMINAL module
 */
uint8_t USART_TERMINAL_read(void)
{
	uint8_t tmptail;

	/* Wait for incoming data */
	while (USART_TERMINAL_rx_elements == 0)
		;
	/* Calculate buffer index */
	tmptail = (USART_TERMINAL_rx_tail + 1) & USART_TERMINAL_RX_BUFFER_MASK;
	/* Store new index */
	USART_TERMINAL_rx_tail = tmptail;
	ENTER_CRITICAL(R);
	USART_TERMINAL_rx_elements--;
	EXIT_CRITICAL(R);

	/* Return data */
	return USART_TERMINAL_rxbuf[tmptail];
}

/**
 * \brief Write one character to USART_TERMINAL
 *
 * Function will block until a character can be accepted.
 *
 * \param[in] data The character to write to the USART
 *
 * \return Nothing
 */
void USART_TERMINAL_write(const uint8_t data)
{
	uint8_t tmphead;

	/* Calculate buffer index */
	tmphead = (USART_TERMINAL_tx_head + 1) & USART_TERMINAL_TX_BUFFER_MASK;
	/* Wait for free space in buffer */
	while (USART_TERMINAL_tx_elements == USART_TERMINAL_TX_BUFFER_SIZE)
		;
	/* Store data in buffer */
	USART_TERMINAL_txbuf[tmphead] = data;
	/* Store new index */
	USART_TERMINAL_tx_head = tmphead;
	ENTER_CRITICAL(W);
	USART_TERMINAL_tx_elements++;
	EXIT_CRITICAL(W);
	/* Enable UDRE interrupt */
	USART2.CTRLA |= (1 << USART_DREIE_bp);
}

/**
 * \brief Initialize USART interface
 * If module is configured to disabled state, the clock to the USART is disabled
 * if this is supported by the device's clock system.
 *
 * \return Initialization status.
 * \retval 0 the USART init was successful
 * \retval 1 the USART init was not successful
 */
int8_t USART_TERMINAL_init()
{

	USART2.BAUD = (uint16_t)USART2_BAUD_RATE(115200); /* set baud rate register */

	USART2.CTRLA = 0 << USART_ABEIE_bp    /* Auto-baud Error Interrupt Enable: disabled */
	               | 0 << USART_DREIE_bp  /* Data Register Empty Interrupt Enable: disabled */
	               | 0 << USART_LBME_bp   /* Loop-back Mode Enable: disabled */
	               | USART_RS485_OFF_gc   /* RS485 Mode disabled */
	               | 1 << USART_RXCIE_bp  /* Receive Complete Interrupt Enable: enabled */
	               | 0 << USART_RXSIE_bp  /* Receiver Start Frame Interrupt Enable: disabled */
	               | 0 << USART_TXCIE_bp; /* Transmit Complete Interrupt Enable: disabled */

	USART2.CTRLB = 0 << USART_MPCM_bp       /* Multi-processor Communication Mode: disabled */
	               | 0 << USART_ODME_bp     /* Open Drain Mode Enable: disabled */
	               | 1 << USART_RXEN_bp     /* Reciever enable: enabled */
	               | USART_RXMODE_NORMAL_gc /* Normal mode */
	               | 0 << USART_SFDEN_bp    /* Start Frame Detection Enable: disabled */
	               | 1 << USART_TXEN_bp;    /* Transmitter Enable: enabled */

	// USART2.CTRLC = USART_CMODE_ASYNCHRONOUS_gc /* Asynchronous Mode */
	//		 | USART_CHSIZE_8BIT_gc /* Character size: 8 bit */
	//		 | USART_PMODE_DISABLED_gc /* No Parity */
	//		 | USART_SBMODE_1BIT_gc; /* 1 stop bit */

	USART2.DBGCTRL = 1 << USART_DBGRUN_bp; /* Debug Run: enabled */

	// USART2.EVCTRL = 0 << USART_IREI_bp; /* IrDA Event Input Enable: disabled */

	// USART2.RXPLCTRL = 0x0 << USART_RXPL_gp; /* Receiver Pulse Length: 0x0 */

	// USART2.TXPLCTRL = 0x0 << USART_TXPL_gp; /* Transmit pulse length: 0x0 */

	uint8_t x;

	/* Initialize ringbuffers */
	x = 0;

	USART_TERMINAL_rx_tail     = x;
	USART_TERMINAL_rx_head     = x;
	USART_TERMINAL_rx_elements = x;
	USART_TERMINAL_tx_tail     = x;
	USART_TERMINAL_tx_head     = x;
	USART_TERMINAL_tx_elements = x;

	return 0;
}

/**
 * \brief Enable RX and TX in USART_TERMINAL
 * 1. If supported by the clock system, enables the clock to the USART
 * 2. Enables the USART module by setting the RX and TX enable-bits in the USART control register
 *
 * \return Nothing
 */
void USART_TERMINAL_enable()
{
	USART2.CTRLB |= USART_RXEN_bm | USART_TXEN_bm;
}

/**
 * \brief Enable RX in USART_TERMINAL
 * 1. If supported by the clock system, enables the clock to the USART
 * 2. Enables the USART module by setting the RX enable-bit in the USART control register
 *
 * \return Nothing
 */
void USART_TERMINAL_enable_rx()
{
	USART2.CTRLB |= USART_RXEN_bm;
}

/**
 * \brief Enable TX in USART_TERMINAL
 * 1. If supported by the clock system, enables the clock to the USART
 * 2. Enables the USART module by setting the TX enable-bit in the USART control register
 *
 * \return Nothing
 */
void USART_TERMINAL_enable_tx()
{
	USART2.CTRLB |= USART_TXEN_bm;
}

/**
 * \brief Disable USART_TERMINAL
 * 1. Disables the USART module by clearing the enable-bit(s) in the USART control register
 * 2. If supported by the clock system, disables the clock to the USART
 *
 * \return Nothing
 */
void USART_TERMINAL_disable()
{
	USART2.CTRLB &= ~(USART_RXEN_bm | USART_TXEN_bm);
}

/**
 * \brief Get recieved data from USART_TERMINAL
 *
 * \return Data register from USART_TERMINAL module
 */
uint8_t USART_TERMINAL_get_data()
{
	return USART2.RXDATAL;
}
