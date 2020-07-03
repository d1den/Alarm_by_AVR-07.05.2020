/*
 * I2C.h
 * Заголовочный файл для библиотеки I2C
 * Created: 05.05.2020 12:45:49
 */ 


#ifndef I2C_H_
#define I2C_H_
#include <avr/io.h> // Подключаем библиотеку микроконтроллеров
#define F_CPU 16000000L // Частота тактов CPU равна 16 МГц
#define F_SCL 100000L // Частота I2C 100 кГц. Большую не поддерживают часы
#define TW_START 0xA4 // отправка start condition (TWINT,TWSTA,TWEN)
#define TW_READY (TWCR & 0x80) // TWI в состоянии готовности (TWINT вернулся в состояние лог. 1)
#define TW_STATUS (TWSR & 0xF8) // возвращает значение
#define DS1307 0xD0 // адрес по шине I2C для DS1307 RTC
#define TW_SEND 0x84 // команда отправки данных (TWINT,TWEN)
#define TW_STOP 0x94 // послать stop condition (TWINT,TWSTO,TWEN)
#define I2C_Stop() TWCR = TW_STOP // inline-макрос для stop condition
#define TW_NACK 0x84 // чтение данных с NACK (что означает, что это последний байт)
#define READ 1
#define SECONDS_REGISTER   0x00 // Адрес регистра секунд
#define MINUTES_REGISTER   0x01 // Минут
#define HOURS_REGISTER     0x02 // Часов
#define DAYOFWK_REGISTER   0x03 // Дней недели
#define DAYS_REGISTER      0x04 // Дней в месяце
#define MONTHS_REGISTER    0x05 // Месяцев
#define YEARS_REGISTER     0x06 // Лет
typedef unsigned char byte; // Переобъявляем тип byte 
void I2C_Init(); // Иницализируем все методы библиотеки
byte I2C_Start();
byte I2C_SendAddr(byte addr);
byte I2C_Write (byte data);
byte I2C_ReadNACK ();
void I2C_WriteRegister(byte busAddr,byte deviceRegister, byte data);
byte I2C_ReadRegister(byte busAddr, byte deviceRegister);
void DS1307_GetTime(byte *hours, byte *minutes, byte *seconds);
void DS1307_GetDate(byte *months, byte *days, byte *years);

#endif /* I2C_H_ */