/* Target configuration */
#define F_CPU (16000000L)
#define JUMP_ADDR (0x00)

/* SWUART pin-configuration */
#define CFG_SWUART_RX_BIT	(PD0)
#define CFG_SWUART_RX_PORT	(PORTD)
#define CFG_SWUART_RX_PIN	(PIND)
#define CFG_SWUART_RX_DIR	(DDRD)

#define CFG_SWUART_TX_BIT	(PD1) 
#define CFG_SWUART_TX_PORT	(PORTD)
#define CFG_SWUART_TX_DIR	(DDRD)

#define LED_DIR  DDRB
#define LED_PORT PORTB
#define LED_BIT  PB7
