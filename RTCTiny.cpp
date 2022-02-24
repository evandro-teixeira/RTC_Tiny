/**
 * @file RTCTiny.cpp
 * @author Evandro Teixeira
 * @brief 
 * @version 0.1
 * @date 14-02-2022
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <Arduino.h>
#include "RTCTiny.hpp"
#include "Wire.h"

#define DS1307_RAM_MEMORY_START 0x08
#define DS1307_RAM_MEMORY_END   0x3F

typedef union 
{
    uint8_t Data;
    struct 
    {
        unsigned BCD_uni : 4;
        unsigned BCD_dec : 4;
    };
}BCDData_t;

/**
 * @brief Construct a new Rtc Tiny:: Rtc Tiny object
 * 
 * @param addAT24C32 
 * @param addDS1307 
 */
RtcTiny::RtcTiny(uint8_t addAT24C32, uint8_t addDS1307)
{
    addressAT24C32 = addAT24C32;
    addressDS1307 = addDS1307;
}

/**
 * @brief Destroy the Rtc Tiny:: Rtc Tiny object
 * 
 */
RtcTiny::~RtcTiny()
{
}

/**
 * @brief 
 * 
 */
void RtcTiny::Init(void)
{
    Wire.begin() ;
}

/**
 * @brief 
 * 
 * @param add 
 * @param data 
 */
void RtcTiny::WriteROM(uint16_t add, uint8_t data)
{
    uint8_t first = 0; 
    uint8_t Second = 0;

    Wire.beginTransmission( addressAT24C32 );
    GetFirstAndSecondByte(add,&first,&Second);
    Wire.write(first);      // first address byte
    Wire.write(Second);     // second address byte
    Wire.write(data);
    Wire.endTransmission();
    Delay(10);
}

/**
 * @brief 
 * 
 * @param add 
 * @param data 
 */
void RtcTiny::ReadROM(uint16_t add, uint8_t *data)
{
    uint8_t first = 0; 
    uint8_t Second = 0;

    Wire.beginTransmission( addressAT24C32 );
    GetFirstAndSecondByte(add,&first,&Second);
    Wire.write( first );    // first address byte
    Wire.write( Second );   // second address byte
    Wire.endTransmission();
    Delay(10);
    Wire.requestFrom(addressAT24C32, 1);

    *data = Wire.read();
}



/**
 * @brief 
 * 
 * @param type 
 * @param data 
 */
void RtcTiny::WriteRTC( DS1307Data_t data)
{
    uint8_t address = 0;
    Wire.beginTransmission( addressDS1307 ) ;

    Wire.write( address++ ) ;                           // address 00h.
    Wire.write( DecimalToBCD(0x3F,data.Seconds) );      // Seconds
    Wire.write( address++ ) ;                           // address 01h.
    Wire.write( DecimalToBCD(0x3F,data.Minutes) );      // Minutes
    Wire.write( address++ ) ;                           // address 02h.
    Wire.write( 0x40 | DecimalToBCD(0x3F,data.Hours) ); // Hours 24
    Wire.write( address++ ) ;                           // address 03h.
    Wire.write( DecimalToBCD(0x07,data.Day) );          // Day
    Wire.write( address++ ) ;                           // address 04h.
    Wire.write( DecimalToBCD(0x1F,data.Date) );         // Date
    Wire.write( address++ ) ;                           // address 05h.
    Wire.write( DecimalToBCD(0x0F,data.Month) );        // Month
    Wire.write( address++ ) ;                           // address 06h.
    Wire.write( DecimalToBCD(0x7F,data.Year) );         // Month

    Wire.endTransmission() ;
}

/**
 * @brief 
 * 
 * @param data 
 */
void RtcTiny::ReadRTC( DS1307Data_t *data)
{
    uint8_t buffer[7] = {0};
    uint8_t i = 0;

    Wire.beginTransmission( addressDS1307 ) ;
    Wire.write( 0x00 ) ; 
    Wire.endTransmission();
    Wire.requestFrom( addressDS1307 , 7 ) ;
    for(i=0;i<7;i++)
    {
        buffer[i]= Wire.read();
    }
    data->Seconds = BCDToDecimal(0x70 & buffer[0], 0x0F & buffer[0]); 
    data->Minutes = BCDToDecimal(0x70 & buffer[1], 0x0F & buffer[1]);
    data->Hours   = BCDToDecimal(0x30 & buffer[2], 0x0F & buffer[2]);
    data->Day     = BCDToDecimal(0x00 & buffer[3], 0x07 & buffer[3]); 
    data->Date    = BCDToDecimal(0x30 & buffer[4], 0x0F & buffer[4]); 
    data->Month   = BCDToDecimal(0x10 & buffer[5], 0x0F & buffer[5]); 
    data->Year    = BCDToDecimal(0xF0 & buffer[6], 0x0F & buffer[6]); 
}

/**
 * @brief 
 * 
 * @param add 
 * @param data 
 */
void RtcTiny::WriteRAM(uint16_t add, uint8_t data)
{
    if((add >= DS1307_RAM_MEMORY_START) && (add <= DS1307_RAM_MEMORY_END))
    {
        Wire.beginTransmission( addressDS1307 ) ;
        Wire.write( add ); 
        Wire.write( data ); 
        Wire.endTransmission() ;
    }
}
/**
 * @brief 
 * 
 * @param add 
 * @param data 
 */
void RtcTiny::ReadRAM(uint16_t add, uint8_t *data)
{
    if((add >= DS1307_RAM_MEMORY_START) && (add <= DS1307_RAM_MEMORY_END))
    {
        Wire.beginTransmission( addressDS1307 ) ;
        Wire.write( add ); 
        Wire.endTransmission();
        Wire.requestFrom( addressDS1307 , 1 ) ;
        *data = Wire.read();
        Wire.endTransmission() ;
    }
}

/**
 * @brief 
 * 
 * @param mask 
 * @param value 
 * @return uint8_t 
 */
uint8_t RtcTiny::DecimalToBCD(uint8_t mask, uint8_t value)
{
    BCDData_t var;
    var.Data = 0;
    value  = mask & value;
    var.BCD_dec = value / 10;
    var.BCD_uni = value % 10;
    return var.Data;
}

/**
 * @brief 
 * 
 * @param dec 
 * @param uin 
 * @return uint8_t 
 */
uint8_t RtcTiny::BCDToDecimal(uint8_t dec, uint8_t uin)
{
    uint8_t var = 0;
    dec = dec >> 4;
    var = ((dec * 10) + uin);
    return var;
}

/**
 * @brief Take first and second bit
 * 
 * @param add 
 * @param addFirst 
 * @param addSecond 
 */
void RtcTiny::GetFirstAndSecondByte(uint16_t add, uint8_t *addFirst, uint8_t *addSecond) 
{
    *addFirst = (uint8_t)((add & 0xFF00)>>8);
    *addSecond = (uint8_t)(add & 0x00FF);
}

/**
 * @brief 
 * 
 * @param t 
 */
void RtcTiny::Delay(uint32_t t)
{
    delayMicroseconds( t * 1000 );
}