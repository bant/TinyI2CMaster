//=============================================================================
// File Name    : ST7032i.c
//
// Title        : ST7032i I2C LCD ドライバ
// Revision     : 0.1
// Notes        :
// Target MCU   : AVR ATMega328
// Tool Chain   :
//
// Revision History:
// When         Who         Description of change
// -----------  ----------- -----------------------
// 2013/02/06   ばんと      修正完了
//=============================================================================

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "delay.h"
#include "ST7032i.h"
#include "TinyI2CMaster.h"

/* local typedef -------------------------------------------------------------*/
//アイコンのアドレスとビットの関係
#ifdef STRAWBERRY_LINUX_16x2_LCD
typedef struct{
	uint8_t adr;
	uint8_t data;
}T_ICON;
#endif

/* local define --------------------------------------------------------------*/
/* local macro ---------------------------------------------------------------*/
/* local variables -----------------------------------------------------------*/
uint8_t _display_basic;
uint8_t _display_extended;
uint8_t _displaymode;
uint8_t _displaycontrol;
uint8_t _rab;

#ifdef STRAWBERRY_LINUX_16x2_LCD
const uint8_t Icon_Table[9][2] = {
	{0x00, 0b10000},
	{0x02, 0b10000},
	{0x04, 0b10000},
	{0x06, 0b10000},
	{0x07, 0b10000},
	{0x07, 0b01000},
	{0x09, 0b10000},
	{0x0B, 0b10000},
	{0x0F, 0b10000}
};
#endif

/* local function prototypes -------------------------------------------------*/

/*======================================*/
/*  ST7032i 書き込み関数				*/
/*======================================*/
uint8_t ST7032i_Write( uint8_t data, uint8_t mode )
{
	uint8_t buf[2], i;
	uint8_t rc;		// リトライカウント

	rc = 1;							// Retry out
    for(i = 0; i < RETRY_ST7032I; i++)
    {
		buf[0] = mode;				// モード
		buf[1] = data;				// データ
		if ((rc = TinyI2C_write_data(ST7032I_ADDR, buf, sizeof(buf), SEND_STOP)) != 0)
		{
			continue;
		}
		else
		{
			break;
		}
	}

	return rc;
}

/*======================================*/
/*  ST7032i ポート初期化関数			*/
/*======================================*/
#ifdef USE_ST7032I_INIT_PORT
void ST7032i_InitPort( void )
{
	ST7032i_WAKE_UP_DDR = _BV(ST7032i_WAKE_UP_DDR_NO);		//
	ST7032i_WAKE_UP_PORT &= ~_BV(ST7032i_WAKE_UP_PORT_NO);	// 

	wait_ms(1);
}
#endif

/*======================================*/
/*  ST7032i ウェイクアップ関数			*/
/*======================================*/
#ifdef USE_ST7032I_WAKEUP
void ST7032i_WakeUp( void )
{
	ST7032i_WAKE_UP_PORT &= ~_BV(ST7032i_WAKE_UP_PORT_NO);
	wait_ms(1);

	ST7032i_WAKE_UP_PORT |= _BV(ST7032i_WAKE_UP_PORT_NO);
	wait_ms(10);
}
#endif

/*======================================*/
/*  ST7032i 初期化関数                  */
/*======================================*/
void ST7032i_Init( void )
{
	uint8_t contrast = 45;

	_display_basic = LCD_INSTRUCTION_SET_BASIC | LCD_8BITMODE | LCD_1LINE | LCD_5x8DOTS;
	_display_extended = LCD_INSTRUCTION_SET_EXTENDED | LCD_8BITMODE | LCD_1LINE | LCD_5x8DOTS;
	_displaycontrol = LCD_DISPLAYON;
	_display_basic |= LCD_2LINE;
	_display_extended |= LCD_2LINE;

#ifdef USE_ST7032I_INIT_PORT
	ST7032i_InitPort( );
#endif

#ifdef USE_ST7032I_WAKEUP
	ST7032i_WakeUp( );
#endif
    wait_ms(40);

    // function set  basic
    ST7032i_WriteCmd(LCD_FUNCTIONSET | _display_basic );
    wait_ms(30);

    // function set extended
    ST7032i_WriteCmd(LCD_FUNCTIONSET | _display_extended);
    wait_ms(30);

    // interval osc
    ST7032i_WriteCmd(LCD_BIAS_OSC_CONTROL | LCD_BIAS1_5 | LCD_OSC_192);
    wait_ms(30);

    // contrast low nible
    ST7032i_WriteCmd(LCD_CONTRAST_LOW_BYTE | (contrast & LCD_CONTRAST_LOW_BYTE_MASK));
    wait_ms(30);

    // contrast high nible / icon / power
    ST7032i_WriteCmd(LCD_ICON_CONTRAST_HIGH_BYTE | LCD_ICON_ON | LCD_BOOSTER_ON | (contrast >> 4 & LCD_CONTRAST_HIGH_BYTE_MASK));
    wait_ms(30);

    // follower control
    _rab = LCD_Rab_2_00;
    ST7032i_WriteCmd(LCD_FOLLOWER_CONTROL | LCD_FOLLOWER_ON | _rab);
    wait_ms(200);

    // function set basic
    ST7032i_WriteCmd(LCD_FUNCTIONSET | _display_basic);
    wait_ms(30);

    // display on
    ST7032i_WriteCmd(LCD_DISPLAYCONTROL |  LCD_DISPLAYON |  LCD_CURSOROFF | LCD_BLINKOFF );
    wait_ms(30);

    // entry mode set
    _displaymode=LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    ST7032i_WriteCmd(LCD_ENTRYMODESET | _displaymode);
    wait_ms(30);
}

/*======================================*/
/*  ST7032i 画面消去関数                */
/*======================================*/
void ST7032i_Clear( void )
{
    ST7032i_WriteCmd(LCD_CLEARDISPLAY);
    wait_ms(2);
}

/*======================================*/
/*  ST7032i ホームポジション関数		*/
/*======================================*/
void ST7032i_Home( void )
{
    ST7032i_WriteCmd(LCD_RETURNHOME);  // set cursor position to zero
    wait_ms(2);  // this command takes a long time!
}

/*======================================*/
/*  ST7032i カーソル表示関数			*/
/*======================================*/
void ST7032i_setCursor(uint8_t col, uint8_t row)
{
    int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };

    if ( row > ST7032_NUM_LINES )
    {
        row = ST7032_NUM_LINES - 1;    // we count rows starting w/0
    }
    ST7032i_WriteCmd(LCD_SETDDRAMADDR | (col + row_offsets[row]));
    wait_ms(30);
}

/*======================================*/
/*  ST7032i 表示オン関数		        */
/*======================================*/
void ST7032i_onDisplay( void )
{
    _displaycontrol |= LCD_DISPLAYON;
    ST7032i_WriteCmd(LCD_DISPLAYCONTROL | _displaycontrol);
    wait_ms(30);
}

/*======================================*/
/*  ST7032i 表示オフ関数		        */
/*======================================*/
void ST7032i_offDisplay( void )
{
    _displaycontrol &= ~LCD_DISPLAYON;
    ST7032i_WriteCmd(LCD_DISPLAYCONTROL | _displaycontrol);
    wait_ms(30);
}

/*======================================*/
/*  ST7032i カーソルオン関数	        */
/*======================================*/
void ST7032i_onCursor( void )
{
    _displaycontrol |= LCD_CURSORON;
    ST7032i_WriteCmd(LCD_DISPLAYCONTROL | _displaycontrol);
    wait_ms(30);
}

/*======================================*/
/*  ST7032i カーソルオフ関数	        */
/*======================================*/
void ST7032i_offCursor( void )
{
    _displaycontrol &= ~LCD_CURSORON;
    ST7032i_WriteCmd(LCD_DISPLAYCONTROL | _displaycontrol);
    wait_ms(30);
}

/*======================================*/
/*  ST7032i ブリンクオン関数	        */
/*======================================*/
void ST7032i_onBlink( void )
{
    _displaycontrol |= LCD_BLINKON;
    ST7032i_WriteCmd(LCD_DISPLAYCONTROL | _displaycontrol);
    wait_ms(30);
}

/*======================================*/
/*  ST7032i ブリンクオフ関数	        */
/*======================================*/
void ST7032i_offBlink( void )
{
    _displaycontrol &= ~LCD_BLINKON;
    ST7032i_WriteCmd(LCD_DISPLAYCONTROL | _displaycontrol);
    wait_ms(30);
}

/*======================================*/
/*  ST7032i 左スクロール関数	        */
/*======================================*/
// These commands scroll the display without changing the RAM
void ST7032i_scrollDisplayLeft( void )
{
    ST7032i_WriteCmd(LCD_FUNCTIONSET | _display_basic);
    wait_ms(30);

    ST7032i_WriteCmd(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
    wait_ms(30);
}

/*======================================*/
/*  ST7032i 右スクロール関数	        */
/*======================================*/
void ST7032i_scrollDisplayRight( void )
{
    ST7032i_WriteCmd(LCD_FUNCTIONSET | _display_basic);
    wait_ms(30);

    ST7032i_WriteCmd(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
    wait_ms(30);
}

/*======================================*/
/*  ST7032i 左→右・文字あふれ関数		*/
/*======================================*/
// This is for text that flows Left to Right
void ST7032i_leftToRight( void )
{
    _displaymode |= LCD_ENTRYLEFT;
    ST7032i_WriteCmd(LCD_ENTRYMODESET | _displaymode);
    wait_ms(30);
}

/*======================================*/
/*  ST7032i 右→左・文字あふれ関数		*/
/*======================================*/
// This is for text that flows Right to Left
void ST7032i_rightToLeft( void )
{
    _displaymode &= ~LCD_ENTRYLEFT;
    ST7032i_WriteCmd(LCD_ENTRYMODESET | _displaymode);
    wait_ms(30);
}

/*======================================*/
/*  ST7032i オートスクロール・オン関数	*/
/*======================================*/
// This will 'right justify' text from the cursor
void ST7032i_onAutoscroll( void )
{
    _displaymode |= LCD_ENTRYSHIFTINCREMENT;
    ST7032i_WriteCmd(LCD_ENTRYMODESET | _displaymode);
    wait_ms(30);
}

/*======================================*/
/*  ST7032i オートスクロール・オフ関数	*/
/*======================================*/
// This will 'left justify' text from the cursor
void ST7032i_offAutoscroll( void )
{
    _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
    ST7032i_WriteCmd(LCD_ENTRYMODESET | _displaymode);
    wait_ms(30);
}

/*======================================*/
/*  ST7032i ユーザ文字作成関数			*/
/*======================================*/
// Allows us to fill the first 8 CGRAM locations with custom characters
void ST7032i_createChar(uint8_t location, uint8_t charmap[])
{
    int i;

    location &= 0x7; // we only have 8 locations 0-7
    ST7032i_WriteCmd(LCD_SETCGRAMADDR | (location << 3));
    wait_ms(30);

    for (i=0; i<8; i++)
    {
        ST7032i_WriteData(charmap[i]);
        wait_ms(30);
    }
}

/*======================================*/
/*  ST7032i コントラスト設定関数		*/
/*======================================*/
// Set contast level
// The range of 'new_val' depends on the harware configuration of the ST7032i and depends on the follower value, Booster value, Vin and VDD.
// In case of the YMFC-G0802D the follower value is 0x04. According to the info in the ST7032 datasheet this gives us a range of 0 to 10 for the contrast.
// The setting in this library are desigend for and tesed on display YMFC-G0802D ( sold on eBay zyscom http://stores.ebay.com/zyscom?_trksid=p4340.l2563 )
// For other displays other values and ranges may apply.
void ST7032i_setContrast(uint8_t new_val)
{
    ST7032i_WriteCmd(LCD_FUNCTIONSET | _display_extended);
    wait_ms(30);	//																								Needed ??

    ST7032i_WriteCmd(LCD_ICON_CONTRAST_HIGH_BYTE | LCD_ICON_ON | LCD_BOOSTER_ON | (new_val >> 4 & LCD_CONTRAST_HIGH_BYTE_MASK));
    wait_ms(30);

    ST7032i_WriteCmd(LCD_CONTRAST_LOW_BYTE | (new_val & LCD_CONTRAST_LOW_BYTE_MASK));
    wait_ms(30);
}

/*======================================*/
/*  文字例出力関数                      */
/*======================================*/
void ST7032i_puts(const char *s)
{
    register char c;

    while ((c = *s++))
    {
        ST7032i_WriteData(c);
    }
}

/*======================================*/
/*  文字例出力関数2						*/
/*======================================*/
void ST7032i_puts_p(const char *progmem_s)
/* print string from program memory on lcd (no auto linefeed) */
{
    register char c;

    while ( (c = pgm_read_byte(progmem_s++)) )
    {
        ST7032i_WriteData(c);
    }

}/* lcd_puts_p */

#ifdef STRAWBERRY_LINUX_16x2_LCD
/*======================================*/
/*  アイコン表示関数					*/
/*======================================*/
/**
  * @brief  Put icon. value is to be 0 - 12
  * @param  numbet : icon number
  * @retval None
  */
void ST7032i_Icon(uint8_t number, bool flag)
{
	ST7032i_WriteCmd(0b00111001);	// コマンド

	//icon address set
	ST7032i_WriteCmd(0b01000000 | Icon_Table[number][0] );

	if(flag)
	{
		//icon data set
		ST7032i_WriteData(Icon_Table[number][1]);
	}
	else
	{
		//icon data reset
		ST7032i_WriteData(0x00);
	}
	ST7032i_WriteCmd(0b00111000);	//
}

/*======================================*/
/*  電源アイコン表示関数				*/
/*======================================*/
void ST7032i_Power_Icon(uint8_t power, bool flag)
{
	uint8_t tmp;

	tmp = 0b00010;	// 枠
	switch(power)
	{
		case 0:
			break;
		case 1:
			tmp |= 0b10000;
			break;
		case 2:
			tmp |= 0b11000;
			break;
		case 3:
			tmp |= 0b11100;
			break;
		default:
			break;
	}

	ST7032i_WriteCmd(0b00111001);	// コマンド
	//icon address set
	ST7032i_WriteCmd(0b01000000 | 0x0D );
	//icon data set
	if(flag)
	{
		ST7032i_WriteData(tmp);
	}
	else
	{
		ST7032i_WriteData(0x00);
	}
	ST7032i_WriteCmd(0b00111000);	//
}
#endif