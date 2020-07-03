/*
 * I2C.c
 * Библиотека для работы с микросхемой DS1307 через I2C
 * Created: 05.05.2020 12:44:49ъ
 */ 
#include "I2C.h" // Подключаем заголовочный файл библиотеки
void I2C_Init()
// Настройка передачи I2C
{
	// коэффициент деления прескалера 1 (нет деления частоты F_CPU):
	TWSR = 0;
	// настройка частоты SCL:
	TWBR = ((F_CPU/F_SCL)-16)/2;  // == 72
}


byte I2C_Start()
// Генерация start condition по шине I2C.
{
	TWCR = TW_START;           // отправка send
	while (!TW_READY);         // ожидание завершения
	return (TW_STATUS==0x08);  // возврат 1, если устройство найдено, иначе возврат 0
}

byte I2C_SendAddr(byte addr)
// Послать адрес шины для подчиненного устройства.
{
	TWDR = addr;               // загрузка адреса устройства
	TWCR = TW_SEND;            // и отправка его
	while (!TW_READY);         // ожидание завершения
	return (TW_STATUS==0x18);  // возврат 1, если устройство найдено, иначе возврат 0
}

byte I2C_Write (byte data)
// Отправка данных подчиненному устройству.
{
	TWDR = data;               // загрузка данных для отправки
	TWCR = TW_SEND;            // и передача их
	while (!TW_READY);         // ожидание завершения
	return (TW_STATUS!=0x28);  // возврат 1, если устройство найдено, иначе возврат 0
}


byte I2C_ReadNACK ()    
// Чтение байта данных от подчиненного устройства
{
	TWCR = TW_NACK;      // NACK означает, что будет прочитан последний байт,
	//  больше байт прочитано не будет.
	while (!TW_READY);   // ожидание завершения
	return TWDR;         // возврат прочитанного байта
}

void I2C_WriteRegister(byte busAddr,byte deviceRegister, byte data)
// Записать значение в регистр устройства
{
	I2C_Start(); // генерация старт
	I2C_SendAddr(busAddr);      // послать адрес шины
	I2C_Write(deviceRegister); // послать 1 байт - адрес внутреннего регистра микросхемы
	I2C_Write(data);           // послать 2 байт - данные, которые надо записать
	I2C_Stop(); // Остановить передачу
}

byte I2C_ReadRegister(byte busAddr, byte deviceRegister)
// Чтение данных из регистра устройства
{
	byte data = 0; // Создаём байт данных
	I2C_Start(); // Начало общения с устройством
	I2C_SendAddr(busAddr);      // послать адрес шины
	I2C_Write(deviceRegister); // установка указателя на регистр
	I2C_Start();// Начало общения с устройством
	I2C_SendAddr(busAddr+READ); // перезапуск в качестве операции чтения
	data = I2C_ReadNACK();     // чтение данных регистра
	I2C_Stop(); // Преращение считывания
	return data; // Вывод данных
}

void DS1307_GetTime(byte *hours, byte *minutes, byte *seconds)
// Метод возвращает время с микросхемы часов в формате BCD (в старшем полубайте десятки числа, в младшем - единицы)
{
	*hours = I2C_ReadRegister(DS1307,HOURS_REGISTER); // Считывает значения с регистра, хранящего часы и передаёт их через указатель
	*minutes = I2C_ReadRegister(DS1307,MINUTES_REGISTER); // Минуты
	*seconds = I2C_ReadRegister(DS1307,SECONDS_REGISTER); // Секунды
	if (*hours & 0b01000000) // проверяем, установлен ли режим 12 часов
	*hours &= 0b00011111; // используются младшие 5 бит, с помощью наложения маски на часы
	else
	*hours &= 0b00111111; // режим 24 часов: используются младшие 6 бит
}

void DS1307_GetDate(byte *months, byte *days, byte *years)
// Метод возвращает месяц, даень и год в формате BCD, аналогично предыдущему методу.
{
	*days = I2C_ReadRegister(DS1307,DAYS_REGISTER); // День
	*months = I2C_ReadRegister(DS1307,MONTHS_REGISTER); // Месяц
	*years = I2C_ReadRegister(DS1307,YEARS_REGISTER); // Гож
}
