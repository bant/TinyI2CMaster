//========================================================================
// File Name    : RTC8564.c
//
// Title        : Seiko Epson RTC-8564 ドライバ
// Revision     : 0.2
// Notes        :
// Target MCU   : AVR ATtiny series
// Tool Chain   : AVR toolchain Ver3.4.1.1195
//
// Revision History:
// When         Who         Description of change
// -----------  ----------- -----------------------
// 2013/04/13   ばんと      製作開始
// 2013/04/14   ばんと      Ver0.1製作完了
// 2013/04/26   ばんと      レジスタ操作関数追加&変更
// 2013/05/07   ばんと      TIMER & ALARMのバク修正 Ver0.2
//------------------------------------------------------------------------
// This code is distributed under Apache License 2.0 License
//		which can be found at http://www.apache.org/licenses/
//========================================================================

/* Includes ------------------------------------------------------------*/
#include <avr/io.h>
#include "delay.h"
#include "TinyI2CMaster.h"
#include "rtc8564.h"

/* local define --------------------------------------------------------*/
/* local typedef -------------------------------------------------------*/
/* local macro ---------------------------------------------------------*/
/* local variables -----------------------------------------------------*/
/* local function prototypes -------------------------------------------*/
static uint8_t dec2bcd(uint8_t d);
static uint8_t bcd2dec(uint8_t b);

/* [ここからソース] ==================================================== */

//========================================================================
//  convert bin to BCD
//------------------------------------------------------------------------
// 引数:
// 戻値:
//========================================================================
static uint8_t dec2bcd(uint8_t d)
{
    return ((d/10 * 16) + (d % 10));
}

//========================================================================
//  convert BCD to bin
//------------------------------------------------------------------------
// 引数:
// 戻値:
//========================================================================
static uint8_t bcd2dec(uint8_t b)
{
    return ((b/16 * 10) + (b % 16));
}

//========================================================================
//  曜日判定
//------------------------------------------------------------------------
// 引数: int nYear  : 年
//       int nMonth : 月
//       int nDay   : 日
// 戻値: 0:日, ... 6:土
//========================================================================
int getWeekday( int nYear, int nMonth, int nDay )
{
    int nWeekday, nTmp;

    if (nMonth == 1 || nMonth == 2)
    {
        nYear--;
        nMonth += 12;
    }

    nTmp = nYear/100;
    nWeekday = (nYear + (nYear >> 2) - nTmp + (nTmp >> 2) + (13 * nMonth + 8)/5 + nDay) % 7;

    return nWeekday;
}

//========================================================================
// 初期化(アプリケーションマニュアル P-29)
//------------------------------------------------------------------------
// 引数: なし
// 戻値: 0=正常終了 それ以外I2C通信エラー
//========================================================================
uint8_t RTC8564_init( void )
{
    uint8_t data[18];

    data[0] = 0x00;          // write reg addr 00
    data[1] = 0x20;          // 00 Control 1, STOP=1
    data[2] = 0x00;          // 01 Control 2
    data[3] = 0x00;          // 02 Seconds
    data[4] = 0x00;          // 03 Minutes
    data[5] = 0x00;          // 04 Hours
    data[6] = 0x01;          // 05 Days
    data[7] = 0x01;          // 06 Weekdays
    data[8] = 0x01;          // 07 Months
    data[9] = 0x01;          // 08 Years
    data[10] = 0x80;         // 09 Minutes Alarm
    data[11] = 0x80;         // 0A Hours Alarm
    data[12] = 0x80;         // 0B Days Alarm
    data[13] = 0x80;         // 0C Weekdays Alarm
    data[14] = 0x00;         // 0D CLKOUT
    data[15] = 0x00;         // 0E Timer control
    data[16] = 0x00;         // 0F Timer
    data[17] = 0x00;         // 00 Control 1, STOP=0(START)

    return TinyI2C_write_data(I2C_ADDR_RTC8564, data, sizeof(data), SEND_STOP);
}

//========================================================================
// 電源投入時の処理(アプリケーションマニュアル P-29)
//------------------------------------------------------------------------
// 引数: なし
// 戻値: 0=正常終了 それ以外I2C通信エラー
//========================================================================
uint8_t RTC8564_power_on( void )
{
    wait_sec(1);

    return RTC8564_init();

}

//========================================================================
// バックアップ復帰処理(アプリケーションマニュアル P-30)
//------------------------------------------------------------------------
// 引数: なし
// 戻値: 0=正常終了 それ以外I2C通信エラー
//========================================================================
uint8_t RTC8564_backup_return( void )
{
    uint8_t data;
    uint8_t status;

    // Seconds レジスタ
    status = TinyI2C_readReg( I2C_ADDR_RTC8564, 0x02, &data );
    if(status != TINYI2C_NO_ERROR)
    {
        return status;
    }

    if( data & _BV(7) ) /* VLチェック: 電圧降下?*/
    {
        return RTC8564_power_on();
    }

    return status;
}

//========================================================================
// 時計・カレンダの設定(アプリケーションマニュアル P-30)
//------------------------------------------------------------------------
// 引数: RTC_TIME *time: 設定する日時データ
// 戻値: 0=正常終了 それ以外I2C通信エラー
//========================================================================
uint8_t RTC8564_adjust( const RTC_TIME *time )
{
    uint8_t data[8];
    uint8_t status;

    // RTC8564 停止
    status = RTC8564_stop();
    if(status != TINYI2C_NO_ERROR)
    {
        return status;
    }
    //
    data[0] = 0x02;          // レジスタテーブル アドレス(秒)
    data[1] = dec2bcd(time->sec);    // 秒
    data[2] = dec2bcd(time->min);    // 分
    data[3] = dec2bcd(time->hour);   // 時
    data[4] = dec2bcd(time->day);    // 日
    data[5] = dec2bcd(time->wday);   // 曜日
    data[6] = dec2bcd(time->month);  // 月

    if (time->year >= 2100)
    {
        data[7] = dec2bcd(time->year - 2100);   // 年
		data[6] |= 0x80;						// 世紀フラッグセット
    }
    else
    {
        data[7] = dec2bcd(time->year - 2000);   // 年
    }

    status = TinyI2C_write_data(I2C_ADDR_RTC8564, data, sizeof(data), SEND_STOP);
    if(status != TINYI2C_NO_ERROR)
    {
        return status;
    }

    // RTC8564 スタート
    return RTC8564_start();

}

//========================================================================
// 時計・カレンダの読み出し(アプリケーションマニュアル P-30)
//------------------------------------------------------------------------
// 引数: RTC_TIME *time: 取得する日時のデータ
// 戻値: 0=正常終了 それ以外I2C通信エラー
//========================================================================
uint8_t RTC8564_now( RTC_TIME *time )
{
    uint8_t data[7];
    uint8_t status;

    /* 読み込むアドレスの登録 */
    data[0] = 0x02;          // レジスタテーブル アドレス(秒)
    status = TinyI2C_write_data(I2C_ADDR_RTC8564, data, 1, NO_SEND_STOP);
    if(status != TINYI2C_NO_ERROR)
    {
        return status;
    }
    status = TinyI2C_read_data(I2C_ADDR_RTC8564, data, 7, SEND_STOP);
    if(status != TINYI2C_NO_ERROR)
    {
        return status;
    }

    time->sec  = bcd2dec( data[0] & 0x7F );
    time->min  = bcd2dec( data[1] & 0x7F );
    time->hour = bcd2dec( data[2] & 0x3F );
    time->day  = bcd2dec( data[3] & 0x3F );
    time->wday = bcd2dec( data[4] & 0x07 );
    time->month =bcd2dec( data[5] & 0x1F );

    if ( data[5] & 0x80 ) /* 世紀フラグ */
    {
        time->year = bcd2dec( data[6] ) + 2100;
    }
    else
    {
        time->year = bcd2dec( data[6] ) + 2000;
    }

    return status;
}

#ifdef USE_ALARM
//========================================================================
//  タイマ割り込み機能設定(アプリケーションマニュアル P-31)
//------------------------------------------------------------------------
// 引数: uint8_t cycle   : 0なら一度きりの割り込み　非0なら繰り返し割り込み
//       uint8_t int_out : 0なら/INT "LOW"レベル割り込み出力不許可
//                         非0なら/INT "LOW"レベル割り込み出力許可
//       enum RTC_TIMER_TIMING sclk
//          : ソースクロックの指定(244.14us[0] 15.625ms[1] 1sec[2] 1min[3])
//       uint8_t count : カウント値の指定(1-255)
// 戻値: 0=正常終了 それ以外I2C通信エラー
//========================================================================
uint8_t RTC8564_setTimer( enum RTC_TIMER_TIMING sclk, uint8_t count, uint8_t cycle, uint8_t int_out )
{
    uint8_t data[2];
    uint8_t status;

    // タイマ割り込み停止(TE = 0)
    status = TinyI2C_clearRegBit( I2C_ADDR_RTC8564, 0x0E, _BV(7) );
    if(status != TINYI2C_NO_ERROR)
    {
        return status;
    }

    // 割り込み解除( TIE=0, TF=0 )
    status = TinyI2C_clearRegBit( I2C_ADDR_RTC8564, 0x01, _BV(2) | _BV(0) );
    if(status != TINYI2C_NO_ERROR)
    {
        return status;
    }

    if ( cycle )
    {
        // 繰り返し割り込み( TI/TP = 1 )
        status = TinyI2C_setRegBit( I2C_ADDR_RTC8564, 0x01, _BV(4) );
    }
    else
    {
        // 一度きりの割り込み( TI/TP = 0 )
        status = TinyI2C_clearRegBit( I2C_ADDR_RTC8564, 0x01, _BV(4) );
    }

    if ( int_out )
    {
        // /INT "LOW"レベル割り込み出力許可( TIE = 1 )
        status = TinyI2C_setRegBit( I2C_ADDR_RTC8564, 0x01, _BV(0) );
    }
    else
    {
        // /INT "LOW"レベル割り込み出力不許可( TIE = 0 )
        status = TinyI2C_clearRegBit( I2C_ADDR_RTC8564, 0x01, _BV(0) );
    }
    if(status != TINYI2C_NO_ERROR)
    {
        return status;
    }

    // タイマカウントダウン周期設定
    status = TinyI2C_masksetRegBit( I2C_ADDR_RTC8564, 0x0E, _BV(1) | _BV(0), sclk );
    if(status != TINYI2C_NO_ERROR)
    {
        return status;
    }

    // タイマカウンタ値設定
    data[0] = 0x0F;          // タイマーのレジスタ・アドレス
    data[1] = count;         // カウントダウンタイマ値設定
    status = TinyI2C_write_data(I2C_ADDR_RTC8564, data, 2, SEND_STOP);
    if(status != TINYI2C_NO_ERROR)
    {
        return status;
    }

    return TinyI2C_setRegBit( I2C_ADDR_RTC8564, 0x0E, _BV(7) );    // タイマ割り込み許可(TE = 1)
}

//========================================================================
//  タイマストップ
//------------------------------------------------------------------------
// 引数: なし
// 戻値: 0=正常終了 それ以外I2C通信エラー
//========================================================================
uint8_t RTC8564_stopTimer( void )
{
    uint8_t status;

	// タイマ割り込み停止(TE = 0)
	status = TinyI2C_clearRegBit( I2C_ADDR_RTC8564, 0x0E, _BV(7) );
    if(status != TINYI2C_NO_ERROR)
    {
		return status;
	}

	// 割り込み解除およびフラッグクリア( TIE=0, TF=0 )
	return TinyI2C_clearRegBit( I2C_ADDR_RTC8564, 0x01, _BV(2) | _BV(0) );

}

//========================================================================
//  タイマクリア
//------------------------------------------------------------------------
// 引数: なし
// 戻値: 0=正常終了 それ以外I2C通信エラー
//========================================================================
uint8_t RTC8564_clearTimer( void )
{
	return TinyI2C_clearRegBit( I2C_ADDR_RTC8564, 0x01, _BV(2) );
}

//========================================================================
//  アラーム設定・開始
//------------------------------------------------------------------------
// 引数: ALARM_TIME *alarm : アラームの設定データ
// 戻値: 0=正常終了 それ以外I2C通信エラー
//========================================================================
uint8_t RTC8564_setAlarm( ALARM_TIME *alarm )
{
    uint8_t data[5];
    uint8_t status;

    // アラーム割り込み停止(AE = 1)
    data[0] = 0x09;         // Minute Alarmレジスタ・アドレス
    data[1] = 0x80;         // Minute Alarm (AE=1)
    data[2] = 0x80;         // Hour Alarm (AE=1)
    data[3] = 0x80;         // Day Alarm (AE=1)
    data[4] = 0x80;         // Week Day Alarm (AE=1)
    status = TinyI2C_write_data(I2C_ADDR_RTC8564, data, 5, SEND_STOP);
    if(status != TINYI2C_NO_ERROR)
    {
        return status;
    }

    // 割り込み解除( AIE=0, AF=0 )
    status = TinyI2C_clearRegBit( I2C_ADDR_RTC8564, 0x01, _BV(3) | _BV(1) );
    if(status != TINYI2C_NO_ERROR)
    {
        return status;
    }

    data[0] = 0x09;              // 分アラームレジスタ・アドレス
    // 毎分設定
    data[1] = dec2bcd(alarm->min & 0x7F);
    if(alarm->min & 0x80)
    {
        data[1] |= 0x80;
    }

    // 毎時設定
    data[2] = dec2bcd(alarm->hour & 0x7F);
    if(alarm->hour & 0x80)
    {
        data[2] |= 0x80;
    }

    // 毎日設定
    data[3] = dec2bcd(alarm->day & 0x7F);
    if(alarm->day & 0x80)
    {
        data[3] |= 0x80;
    }

    // 毎曜日設定
    data[4] = dec2bcd(alarm->wday & 0x7F);
    if(alarm->wday & 0x80)
    {
        data[4] |= 0x80;
    }

    status = TinyI2C_write_data(I2C_ADDR_RTC8564, data, 5, SEND_STOP);
    if(status != TINYI2C_NO_ERROR)
    {
        return status;
    }

    // 割り込み許可(AIE=1)
    return TinyI2C_setRegBit( I2C_ADDR_RTC8564, 0x01, _BV(1) );
}

//========================================================================
// アラームデータの読み出し(アプリケーションマニュアル P-30)
//------------------------------------------------------------------------
// 引数: ALARM_TIME *alarm: 取得するアラームのデータ
// 戻値: 0=正常終了 それ以外I2C通信エラー
//========================================================================
uint8_t RTC8564_getAlarm( ALARM_TIME *alarm )
{
    uint8_t data[4];
    uint8_t status;

    /* 読み込むアドレスの登録 */
    data[0] = 0x09;          // レジスタテーブル アドレス(秒)
    status = TinyI2C_write_data(I2C_ADDR_RTC8564, data, 1, NO_SEND_STOP);
    if(status != TINYI2C_NO_ERROR)
    {
        return status;
    }
    status = TinyI2C_read_data(I2C_ADDR_RTC8564, data, 4, SEND_STOP);
    if(status != TINYI2C_NO_ERROR)
    {
        return status;
    }

    alarm->min = bcd2dec( data[0] & 0x7F );
    if ( data[0] & 0x80 )
    {
        alarm->min |= 0x80;
    }

    alarm->hour = bcd2dec( data[1] & 0x3F );
    if ( data[1] & 0x80 )
    {
        alarm->hour |= 0x80;
    }

    alarm->day = bcd2dec( data[2] & 0x3F );
    if ( data[2] & 0x80 )
    {
        alarm->day |= 0x80;
    }

    alarm->wday = bcd2dec( data[3] & 0x07 );
    if ( data[3] & 0x80 )
    {
        alarm->wday |= 0x80;
    }

    return status;
}

//========================================================================
//  アラーム停止
//------------------------------------------------------------------------
// 引数: なし
// 戻値: 0=正常終了 それ以外I2C通信エラー
//========================================================================
uint8_t RTC8564_stopAlarm( void )
{
    uint8_t data[5];
    uint8_t status;
    ALARM_TIME alarm;

	// 割り込み解除( AIE=0, AF=0 )
	status = TinyI2C_clearRegBit( I2C_ADDR_RTC8564, 0x01, _BV(3) | _BV(1) );
    if(status != TINYI2C_NO_ERROR)
    {
		return status;
	}

    status = RTC8564_getAlarm( &alarm );
    if(status != TINYI2C_NO_ERROR)
    {
        return status;
    }

    // アラーム割り込み停止(AE = 1)
    data[0] = 0x09;         // Minute Alarmレジスタ・アドレス
    data[1] = dec2bcd(alarm.min  & 0x7F) | 0x80;         // Minute Alarm (AE=1)
    data[2] = dec2bcd(alarm.hour & 0x7F) | 0x80;         // Hour Alarm (AE=1)
    data[3] = dec2bcd(alarm.day  & 0x7F) | 0x80;         // Day Alarm (AE=1)
    data[4] = dec2bcd(alarm.wday & 0x7F) | 0x80;         // Week Day Alarm (AE=1)
    return TinyI2C_write_data(I2C_ADDR_RTC8564, data, 5, SEND_STOP);
}

//========================================================================
//  アラームクリア
//------------------------------------------------------------------------
// 引数: なし
// 戻値: 0=正常終了 それ以外I2C通信エラー
//========================================================================
uint8_t RTC8564_clearAlarm( void )
{
	return TinyI2C_clearRegBit( I2C_ADDR_RTC8564, 0x01, _BV(3) );
}
#endif

//========================================================================
//  CLKOUTの設定
//------------------------------------------------------------------------
// 引数: enum  RTC_CLKOUT_FREQ clkout: CLKOUTの周波数
// 戻値: 0=正常終了 それ以外I2C通信エラー
//========================================================================
uint8_t RTC8564_setClkOut( enum  RTC_CLKOUT_FREQ clkout )
{
    if( clkout == FREQ_0 )
    {
        return TinyI2C_clearRegBit( I2C_ADDR_RTC8564, 0x0D, _BV(7) );
    }
    else
    {
        return TinyI2C_masksetRegBit( I2C_ADDR_RTC8564, 0x0D, _BV(7) | _BV(1) | _BV(0), _BV(7) | clkout );
    }
}

/* =====================================================[ここまでソース] */
