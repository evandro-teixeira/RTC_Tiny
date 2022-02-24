/**
 * @file RTCTiny.hpp
 * @author Evandro Teixeira
 * @brief 
 * @version 0.1
 * @date 14-02-2022
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <stdint.h>

typedef struct 
{
    uint8_t Seconds;
    uint8_t Minutes;
    uint8_t Hours;
    uint8_t Day;
    uint8_t Date;
    uint8_t Month;
    uint8_t Year;
}DS1307Data_t;


class RtcTiny 
{
private:
    uint8_t addressAT24C32;
    uint8_t addressDS1307;
    uint8_t DecimalToBCD(uint8_t mask, uint8_t value);
    uint8_t BCDToDecimal(uint8_t dec, uint8_t uin);
    void GetFirstAndSecondByte(uint16_t add, uint8_t *addFirst, uint8_t *addSecond); 
    void Delay(uint32_t t);
public:
    RtcTiny(uint8_t addAT24C32, uint8_t addDS1307);
    ~RtcTiny();
    void Init(void);
    void WriteROM(uint16_t add, uint8_t data);
    void ReadROM(uint16_t add, uint8_t *data);
    void WriteRAM(uint16_t add, uint8_t data);
    void ReadRAM(uint16_t add, uint8_t *data);
    void WriteRTC( DS1307Data_t data);
    void ReadRTC( DS1307Data_t *data);
};
