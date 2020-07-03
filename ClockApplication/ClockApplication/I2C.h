/*
 * I2C.h
 * ������������ ���� ��� ���������� I2C
 * Created: 05.05.2020 12:45:49
 */ 


#ifndef I2C_H_
#define I2C_H_
#include <avr/io.h> // ���������� ���������� �����������������
#define F_CPU 16000000L // ������� ������ CPU ����� 16 ���
#define F_SCL 100000L // ������� I2C 100 ���. ������� �� ������������ ����
#define TW_START 0xA4 // �������� start condition (TWINT,TWSTA,TWEN)
#define TW_READY (TWCR & 0x80) // TWI � ��������� ���������� (TWINT �������� � ��������� ���. 1)
#define TW_STATUS (TWSR & 0xF8) // ���������� ��������
#define DS1307 0xD0 // ����� �� ���� I2C ��� DS1307 RTC
#define TW_SEND 0x84 // ������� �������� ������ (TWINT,TWEN)
#define TW_STOP 0x94 // ������� stop condition (TWINT,TWSTO,TWEN)
#define I2C_Stop() TWCR = TW_STOP // inline-������ ��� stop condition
#define TW_NACK 0x84 // ������ ������ � NACK (��� ��������, ��� ��� ��������� ����)
#define READ 1
#define SECONDS_REGISTER   0x00 // ����� �������� ������
#define MINUTES_REGISTER   0x01 // �����
#define HOURS_REGISTER     0x02 // �����
#define DAYOFWK_REGISTER   0x03 // ���� ������
#define DAYS_REGISTER      0x04 // ���� � ������
#define MONTHS_REGISTER    0x05 // �������
#define YEARS_REGISTER     0x06 // ���
typedef unsigned char byte; // ������������� ��� byte 
void I2C_Init(); // ������������� ��� ������ ����������
byte I2C_Start();
byte I2C_SendAddr(byte addr);
byte I2C_Write (byte data);
byte I2C_ReadNACK ();
void I2C_WriteRegister(byte busAddr,byte deviceRegister, byte data);
byte I2C_ReadRegister(byte busAddr, byte deviceRegister);
void DS1307_GetTime(byte *hours, byte *minutes, byte *seconds);
void DS1307_GetDate(byte *months, byte *days, byte *years);

#endif /* I2C_H_ */