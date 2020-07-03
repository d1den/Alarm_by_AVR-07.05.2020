/*
 * I2C.c
 * ���������� ��� ������ � ����������� DS1307 ����� I2C
 * Created: 05.05.2020 12:44:49�
 */ 
#include "I2C.h" // ���������� ������������ ���� ����������
void I2C_Init()
// ��������� �������� I2C
{
	// ����������� ������� ���������� 1 (��� ������� ������� F_CPU):
	TWSR = 0;
	// ��������� ������� SCL:
	TWBR = ((F_CPU/F_SCL)-16)/2;  // == 72
}


byte I2C_Start()
// ��������� start condition �� ���� I2C.
{
	TWCR = TW_START;           // �������� send
	while (!TW_READY);         // �������� ����������
	return (TW_STATUS==0x08);  // ������� 1, ���� ���������� �������, ����� ������� 0
}

byte I2C_SendAddr(byte addr)
// ������� ����� ���� ��� ������������ ����������.
{
	TWDR = addr;               // �������� ������ ����������
	TWCR = TW_SEND;            // � �������� ���
	while (!TW_READY);         // �������� ����������
	return (TW_STATUS==0x18);  // ������� 1, ���� ���������� �������, ����� ������� 0
}

byte I2C_Write (byte data)
// �������� ������ ������������ ����������.
{
	TWDR = data;               // �������� ������ ��� ��������
	TWCR = TW_SEND;            // � �������� ��
	while (!TW_READY);         // �������� ����������
	return (TW_STATUS!=0x28);  // ������� 1, ���� ���������� �������, ����� ������� 0
}


byte I2C_ReadNACK ()    
// ������ ����� ������ �� ������������ ����������
{
	TWCR = TW_NACK;      // NACK ��������, ��� ����� �������� ��������� ����,
	//  ������ ���� ��������� �� �����.
	while (!TW_READY);   // �������� ����������
	return TWDR;         // ������� ������������ �����
}

void I2C_WriteRegister(byte busAddr,byte deviceRegister, byte data)
// �������� �������� � ������� ����������
{
	I2C_Start(); // ��������� �����
	I2C_SendAddr(busAddr);      // ������� ����� ����
	I2C_Write(deviceRegister); // ������� 1 ���� - ����� ����������� �������� ����������
	I2C_Write(data);           // ������� 2 ���� - ������, ������� ���� ��������
	I2C_Stop(); // ���������� ��������
}

byte I2C_ReadRegister(byte busAddr, byte deviceRegister)
// ������ ������ �� �������� ����������
{
	byte data = 0; // ������ ���� ������
	I2C_Start(); // ������ ������� � �����������
	I2C_SendAddr(busAddr);      // ������� ����� ����
	I2C_Write(deviceRegister); // ��������� ��������� �� �������
	I2C_Start();// ������ ������� � �����������
	I2C_SendAddr(busAddr+READ); // ���������� � �������� �������� ������
	data = I2C_ReadNACK();     // ������ ������ ��������
	I2C_Stop(); // ���������� ����������
	return data; // ����� ������
}

void DS1307_GetTime(byte *hours, byte *minutes, byte *seconds)
// ����� ���������� ����� � ���������� ����� � ������� BCD (� ������� ��������� ������� �����, � ������� - �������)
{
	*hours = I2C_ReadRegister(DS1307,HOURS_REGISTER); // ��������� �������� � ��������, ��������� ���� � ������� �� ����� ���������
	*minutes = I2C_ReadRegister(DS1307,MINUTES_REGISTER); // ������
	*seconds = I2C_ReadRegister(DS1307,SECONDS_REGISTER); // �������
	if (*hours & 0b01000000) // ���������, ���������� �� ����� 12 �����
	*hours &= 0b00011111; // ������������ ������� 5 ���, � ������� ��������� ����� �� ����
	else
	*hours &= 0b00111111; // ����� 24 �����: ������������ ������� 6 ���
}

void DS1307_GetDate(byte *months, byte *days, byte *years)
// ����� ���������� �����, ����� � ��� � ������� BCD, ���������� ����������� ������.
{
	*days = I2C_ReadRegister(DS1307,DAYS_REGISTER); // ����
	*months = I2C_ReadRegister(DS1307,MONTHS_REGISTER); // �����
	*years = I2C_ReadRegister(DS1307,YEARS_REGISTER); // ���
}
