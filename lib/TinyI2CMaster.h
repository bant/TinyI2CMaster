//========================================================================
// File Name    : TinyI2CMaster.h
//
// Title        : ATtiny用 USIを使ったI2Cドライバ・ヘッダファイル
// Revision     : 0.11
// Notes        :
// Target MCU   : AVR ATtiny series
// Tool Chain   : AVR toolchain Ver3.4.1.1195
//
// Revision History:
// When         Who         Description of change
// -----------  ----------- -----------------------
// ????/??/??   がた老さん  soft_I2C.c開発完了
// 2013/04/10   ばんと      修正完了
// 2013/04/26   ばんと      レジスタ操作関数追加&変更
//------------------------------------------------------------------------
// This code is distributed under Apache License 2.0 License
//		which can be found at http://www.apache.org/licenses/
//========================================================================

#ifndef __TINYI2CMASTER_H_
#define __TINYI2CMASTER_H_

/* define --------------------------------------------------------------*/
#define NOISE_TESTING
#define SIGNAL_VERIFY
#define USE_READ_WRITE_REPEAT
#define USE_READ_WRITE_REGISTER
 
#define RETRY	3

#define T2_TWI    5 		// >4,7us
#define T4_TWI    4 		// >4,0us

#define TINYI2C_NO_ERROR			0x00
#define TINYI2C_UNKNOWN_START		0x01
#define TINYI2C_UNKNOWN_STOP		0x02
#define TINYI2C_DATA_COLLISION		0x03
#define TINYI2C_MISS_START_COND		0x04
#define TINYI2C_MISS_STOP_COND		0x05
#define TINYI2C_SLAVE_NACK			0x06

#define NO_SEND_STOP			0
#define SEND_STOP				1


// Device dependant defines ADDED BACK IN FROM ORIGINAL ATMEL .H

#if defined(__AVR_AT90Mega169__) | defined(__AVR_ATmega169__) | \
	defined(__AVR_AT90Mega165__) | defined(__AVR_ATmega165__) | \
	defined(__AVR_ATmega325__) | defined(__AVR_ATmega3250__) | \
	defined(__AVR_ATmega645__) | defined(__AVR_ATmega6450__) | \
	defined(__AVR_ATmega329__) | defined(__AVR_ATmega3290__) | \
	defined(__AVR_ATmega649__) | defined(__AVR_ATmega6490__)
#define DDR_USI             DDRE
#define PORT_USI            PORTE
#define PIN_USI             PINE
#define PORT_USI_SDA        PORTE5
#define PORT_USI_SCL        PORTE4
#define PIN_USI_SDA         PINE5
#define PIN_USI_SCL         PINE4
#endif

#if defined(__AVR_ATtiny25__) | defined(__AVR_ATtiny45__) | defined(__AVR_ATtiny85__) | \
	defined(__AVR_AT90Tiny26__) | defined(__AVR_ATtiny26__) | defined(__AVR_ATtiny861A__)
#define DDR_USI             DDRB
#define PORT_USI            PORTB
#define PIN_USI             PINB
#define PIN_USI_SDA         PINB0
#define PIN_USI_SCL         PINB2
#endif

#if defined(__AVR_AT90Tiny2313__) | defined(__AVR_ATtiny2313__)
#define DDR_USI             DDRB
#define PORT_USI            PORTB
#define PIN_USI             PINB
#define PIN_USI_SDA         PINB5
#define PIN_USI_SCL         PINB7
#endif

/* typedef -------------------------------------------------------------*/
/* macro ---------------------------------------------------------------*/
/* variables -----------------------------------------------------------*/
/* function prototypes -------------------------------------------------*/
void TinyI2C_Master_init( void );
uint8_t TinyI2C_start( void );
uint8_t TinyI2C_stop( void );
uint8_t TinyI2C_read( uint8_t ack_nack );
uint8_t TinyI2C_write( uint8_t data );
uint8_t TinyI2C_Transfer( uint8_t data );
uint8_t TinyI2C_read_data(uint8_t slave_7bit_addr, void* data, int size, uint8_t send_stop);
uint8_t TinyI2C_write_data(uint8_t slave_7bit_addr, void* data, int size, uint8_t send_stop);
uint8_t TinyI2C_readReg( uint8_t slave_7bit_addr, uint8_t mem_addr, uint8_t *data );
uint8_t TinyI2C_masksetRegBit( uint8_t slave_7bit_addr, uint8_t mem_addr, uint8_t mask, uint8_t set_bit );
uint8_t TinyI2C_setRegBit( uint8_t slave_7bit_addr, uint8_t mem_addr, uint8_t set_bit );
uint8_t TinyI2C_clearRegBit( uint8_t slave_7bit_addr, uint8_t mem_addr, uint8_t clear_bit );

#endif /* TINYI2CMASTER_H_ */
