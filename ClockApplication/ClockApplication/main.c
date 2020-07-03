/*
 * ClockApplication.c
 *
 * Created: 03.05.2020 9:52:59
 */ 

#include <avr/io.h> // ����������� ����������� ���������� ����������������� avr
#define F_CPU 16000000L // ������� ������ CPU ����� 16 ���
#include <util/delay.h> // ���������� ���������� ��������
#include <avr/interrupt.h> // ����������� ���������� ����������
#include "lcd.h" // ����������� ���������� ��� ������ � lcd �������
#include "I2C.h" // ����������� ����������� ���������� ��� ������ � I2C �����������

#define UPBUTTON	INT4_vect_num // ����� ��� ��� ��������� ������ ������� ����������� (�� ������� ������� �����)
#define DOWNBUTTON	INT7_vect_num // ���������� ����
#define RIGHTBUTTON	INT5_vect_num // ������
#define LEFTBUTTON  INT6_vect_num // �����
#define FIRSTLINE	0 // ����� ��������� ��� ������ ������ ���� (����)
#define SECONDLINE  6 // ��� ������ (���������)
#define ISLOW		1 // ��������� ��� �������� ���������
typedef struct
// �������� ���������, �������� �������� ������� ����� � ���������� 
{
	byte realHours, realMinutes, realSeconds, alarmHours, alarmMinutes, alarmSeconds;
} Time; // �������������, ����� ���������� �� struct Time, � ����� Time

byte CursorPosition=0; // ����, �������� ������� �������
byte CursorEnable=0; // ����, ����������� ��������� ������� (��� ��������� ��������) - �������� �������������� ��� ���
// ����� ��������� ������, �������� �������� ��������� ���� (� ������ ����������� ����� ����)
byte CursorPositionMas[12][2]={{6,0},{7,0},{9,0},{10,0},{12,0},{13,0},{6,1},{7,1},{9,1},{10,1},{12,1},{13,1}};
Time timeVar; // ������ ������ ���������, � ������� ����� ������� ��������
unsigned int beep=0; // ����������, �������� ���������� �������� ��� ������������ ����������
void DS1307_GetRealTime() 
// �����,����������� �������� ����� �� ds1307 � ��������� timeVar
{
	cli(); // ��������� ����������
	// �������� �����, ������������ �������� �������
	DS1307_GetTime(&(timeVar.realHours),&(timeVar.realMinutes),&(timeVar.realSeconds)); 
	sei(); // ��������� ����������
}

void DS1307_SetTime() 
// �����, ������������ ����� � ds1307 �� ��������� timeVar
{
	cli(); // ��������� ����������
	I2C_WriteRegister(DS1307,HOURS_REGISTER, timeVar.realHours); // ���������� � ds1307 ����
	I2C_WriteRegister(DS1307,MINUTES_REGISTER, timeVar.realMinutes); // ������
	I2C_WriteRegister(DS1307,SECONDS_REGISTER, timeVar.realSeconds); // �������
	sei(); // ��������� ����������
}

void TwoDigits(byte data)
// �����, ��������� �������� ����� �� �����.
// �� ����� �������� 2 ����� � ������� BCD, � ������� �� �� ������� LCD.
{
	byte high = data>>4;    // �������� ������� 4 ���� � ������� ������ ������
	LCDdata(high+'0');     // ���������� ������� �����. ���������� ������ '0' � ������ �����,
	// ��� ��� �� ���� ���� ������ �������� ����
	data &= 0b00001111;           // �������� ������� 4 ����
	LCDdata(data+'0');     // ���������� ������� �����
}

void LCDGotoXYNUM(byte Num)
// �����, ����������� ������� ������� � ����������, �������� �������� ��������� �������
{
	LCDGotoXY(CursorPositionMas[Num][0],CursorPositionMas[Num][1]);
}

void ScreenUpdate()
// �����, ��������� ���������� �� �����
{
	cli(); // ��������� ����������
	LCDcursorOFF(); // ��������� ������
	LCDstringXY("Time=",0,0); // ������� �� ������ ������ ����� 
	LCDGotoXYNUM(FIRSTLINE); // ������ ��������� � ���������� ������� �������� ������� 
	TwoDigits(timeVar.realHours); // ���������� ����
	LCDdata(':'); // ������� ���������
	TwoDigits(timeVar.realMinutes); // ������
	LCDdata(':'); // ���������
	TwoDigits(timeVar.realSeconds); // �������
	LCDstringXY("Alarm=",0,1); // ������� �� ��������� ������ �����, ��� ��� ���������
	LCDGotoXYNUM(SECONDLINE); // ��������� � ������ ���������� ����������
	TwoDigits(timeVar.alarmHours); // ����� ���������� ������ ����� ����������
	LCDdata(':');
	TwoDigits(timeVar.alarmMinutes);
	LCDdata(':');
	TwoDigits(timeVar.alarmSeconds);
	LCDGotoXYNUM(CursorPosition); // ������ ��������� � �������, ������� ������
	if(CursorEnable==1) // ���� ������ ����� ��������� ��������
		LCDcursor_on(); // �� �������� �������� �������������� ������
	sei(); // ��������� ����������
}

void Init_MK() 
// �������������� ��������� ��
{
	DDRB=0b00000001; // ������������ 0 ��� ����� b �� �����, ��������� �� ����
	DDRE=0x00; // ����������� ��� ���� ����� E �� ����
	PORTE=0xFF; // ����������� ��� ���� ����� E �� PullUp �����
	EICRB=(1<<ISC71)|(1<<ISC61)|(1<<ISC51)|(1<<ISC41); // ����������� ������� ����������.
	// ������������� ����������� ���� � �������� EICRB ��� 4 ���������� �� ������� ������ (� �������� ������ �� ������)
	EIMSK=(1<<INTF7)|(1<<INTF6)|(1<<INTF5)|(1<<INTF4); // ��������� 4 �������� ���������� INT4-INT7.
	// � ������ ����� (PE4-PE7) ���������� 4 ������, �������������� ����� ������ ���������� ������.
	
	// ��� ������ ������� ���������� ����������� ������ 0
	TCCR0=(1<<WGM0)|(1<<CS02)|(1<<CS01)|(1<<CS00); // ������������� ����� ������ CTC (�� ���������) � ������������ 1024
	OCR0=0xff; // ������������� �������� �������� �� ��������� 255
	
	// ��� �������������� ������ ����������� ������ 1
	TCCR1B=(1<<WGM12)|(0<<CS12)|(1<<CS11)|(1<<CS10); // ����������� ����� ����� CTC � ������������ 64
	OCR1AH=0xff; // ������������� ������������ �������� �� ��������� = 65 535 � 16 ��������� �������
	OCR1AL=0xff;
	DS1307_GetRealTime(); // �������� ����� � �����
	sei(); // ��������� ��������� ����������
}

void EditSeconds(byte *seconds,byte numBut,byte isLow)
// ����� ��� �������������� ������
{
	byte high=*seconds>>4; // �������� ������� ��������
	byte low = *seconds & 0b00001111; // � �������
	if(isLow) // ���� �� ����������� ������� ������
	{
		if(numBut==UPBUTTON) // ���� ������ �����
		low=(low==9)?0:low+1; // �� ���� ���� ����� 9, ������ 0, ����� ����������� �� 1
		if(numBut==DOWNBUTTON) // ���� ����
		low=(low==0)?9:low-1; // �� ���� ��� 0, ������ 9, ����� ��������� �� 1
	}
	else // ���� ������� ������ �����
	{
		if(numBut==UPBUTTON) // ���� �����
		high=(high==5)?0:high+1; // �� ���� ���� 5, ������ 0, ����� +1
		if(numBut==DOWNBUTTON) // ���� ����
		high=(high==0)?5:high-1; // �� ���� ��� 0, ������ 5, ����� -1
	}
	*seconds=(high<<4)+low; // ���������� �������� ������, ��������� ������� ������ (��������� ����� �� 4) � �������
}

void EditMinutes(byte *minutes,byte numBut,byte isLow)
// ����� ��� �������������� ������
{
	// �� ���������� ������ ��� �������������� ������
	byte high=*minutes>>4;
	byte low = *minutes & 0b00001111;
	if(isLow)
	{
		if(numBut==UPBUTTON)
		low=(low==9)?0:low+1;
		if(numBut==DOWNBUTTON)
		low=(low==0)?9:low-1;
	}
	else
	{
		if(numBut==UPBUTTON)
		high=(high==5)?0:high+1;
		if(numBut==DOWNBUTTON)
		high=(high==0)?5:high-1;
	}
	*minutes=(high<<4)+low;
}

void EditHour(byte *hours,byte numBut, byte isLow)
// ����� ��� �������������� �����
{
	byte high=*hours>>4; // ������� ������
	byte low = *hours & 0b00001111; // �������
	if(isLow) // ���� ����������� �������
	{
		if(high==2) // ���� ������� ������ = 2
		{
			if(numBut==UPBUTTON) // ���� ������ �����
			low=(low==3)?0:low+1; // �� ���� ������� = 3, ������ 0, ����� +1
			if(numBut==DOWNBUTTON) // ���� ����
			low=(low==0)?3:low-1; // �� ���� ������� = 0, ������ 3, ����� -1
		}
		else // ���� ������� �� 2
		{
			if(numBut==UPBUTTON) // ���� �����
			low=(low==9)?0:low+1; // ���� ������� = 9, �� ������ 0, ����� +1
			if(numBut==DOWNBUTTON) // ���� ����
			low=(low==0)?9:low-1; // �� ���� ������� 0, ������ 9, ����� -1
		}
	}
	else // ���� �� ����������� �������
	{
		if(numBut==UPBUTTON) // ���� �����
		{
			if(high==1) // ���� ������� ������ = 1
			{
				high=2; // ������ 2
				low=(low>3)?3:low; // ��� ������� ��� > 3, �� ������ 3, ����� ��������� ��� ���� 
				// (�� � ����� �� ����� ������������ ������ 23)
			}
			else if(high==2) // ���� �� ������� ������ 2
			high=0; // �� ������ 0
			else // ����� �������� �� 1
			high++;
			
		}
		if(numBut==DOWNBUTTON) // ���� ����
		{
			if(high==0) // ���� 0
			{
				high=2; // ������ 2
				low=(low>3)?3:low; // ���� ������� > 3, �� ������ 3, ����� ���������
			}
			else if(high==0) // ���� ��� 0
			high=2; // ������������� 2
			else // ����� ������ ���������
			high--;
		}
	}
	*hours=(high<<4)+low; // ���������� ����, ��������� �� �������� � �������� �������
}
void Redact(byte numBut, byte position)
// ����� �������������� ���������� ��������
{
	switch(position)
	// ���������, ����� ������� �����������
	{		
		case 0:
			EditHour(&timeVar.realHours,numBut,!ISLOW); // ������� ������ �����
		break;
		case 1:
			EditHour(&timeVar.realHours,numBut,ISLOW); // ������� ������ �����
		break;
		case 2:
			EditMinutes(&timeVar.realMinutes,numBut,!ISLOW); // ���������� � ��������
		break;
		case 3:
			EditMinutes(&timeVar.realMinutes,numBut,ISLOW);
		break;
		case 4:
			EditSeconds(&timeVar.realSeconds,numBut,!ISLOW); // � � ���������
		break;
		case 5:
			EditSeconds(&timeVar.realSeconds,numBut,ISLOW);
		break;
		case 6:
			EditHour(&timeVar.alarmHours,numBut,!ISLOW); // ����� ����������� ����� ���������� ����������
		break;
		case 7:
			EditHour(&timeVar.alarmHours,numBut,ISLOW);
		break;
		case 8:
			EditMinutes(&timeVar.alarmMinutes,numBut,!ISLOW);
		break;
		case 9:
			EditMinutes(&timeVar.alarmMinutes,numBut,ISLOW);
		break;
		case 10:
			EditSeconds(&timeVar.alarmSeconds,numBut,!ISLOW);
		break;
		case 11:
			EditSeconds(&timeVar.alarmSeconds,numBut,ISLOW);
		break;
	}
	DS1307_SetTime(); // ���������� ����������������� ����� � ����
}

void PressButton(byte NumButton)
// �����, ����������� �������� ��� ������� �� �������
{
	cli(); // ��������� ����������
	TCNT1H=0x00; // ���������� �������� ������� 1 ��� ����������� ������ ��������������
	TCNT1L=0x00;
	TIMSK=(0<<OCIE0); // ��������� ������ 0 (������ ���������� ��� ������� �� ����� ������)
	sei(); // ��������� ����������
	if(CursorEnable==0) // ���� ��� �� ��� ������� ����� ��������������
	{
		CursorEnable=1; // �� �������� ���
		CursorPosition=0; // ������������� ������� 0
		cli();
		TIMSK=(1<<OCIE1A); // �������� ������ ������� ������ ��������������
		sei();
		LCDGotoXYNUM(CursorPosition); // ��������� ������ � ������ ������� 
	}
	else // ���� ��� ������
	{
		switch(NumButton)
		// �� � ����������� �� ������� ������ �������� ������ �������
		{
			// ������ �����
			case UPBUTTON:
				Redact(UPBUTTON,CursorPosition); // �������������� ��� ������� ������ �����
			break;
			case RIGHTBUTTON: // ���� ������
				if(CursorPosition==11) // ���� ������� �������, �� ��������� � 0
					CursorPosition=0;
				else
					CursorPosition++; // ����� ����������� �� 1
				LCDGotoXYNUM(CursorPosition); // ��������� ������
			break;
			case LEFTBUTTON: // ���� �����
				if(CursorPosition==0) // ���� 0, �� ������� 11
					CursorPosition=11;
				else
					CursorPosition--; // ����� -1
				LCDGotoXYNUM(CursorPosition); //��������� ������
			break;
			case DOWNBUTTON: // ���� ����
				Redact(DOWNBUTTON,CursorPosition); // �� ��������� ��������
			break;
		}
	}
}

ISR(TIMER1_COMPA_vect)
// ���������� �� ��������� ������� 1
// ��������, ��� �� ���� ������� �� ������ � ������� ~3�,
// � ����� �������������� �����������
{
	CursorEnable=0; // ��������� ����� ��������������
	LCDcursorOFF(); // ��������� ������
	TIMSK=(0<<OCIE1A); // ��������� ������ 1
}

ISR(TIMER0_COMP_vect)
// ���������� �� ��������� ������� 0.
// ������������ ��� �������� ������������� ������� ��������
// � ������� ������� ��� ������������ ���������
{
	beep++; // ����������� ���������� ������������ �������
	if(beep%2==0) // ����� ��� ���������
		LCDblank();
	else // ��� �������� �����
		LCDnblank();
	for(int i=0;i<10;i++) // ������ � ������� ����� � �������� ���������� ��������
	// ������ ������� �������, ����� ��� ������ ���� (~6,66 ���)
	{
		if(i%2==0)
			PORTB=0b0000001; // ����� ��� ����� �� ������� 1
		else // ��� 0
			PORTB=0;
		_delay_us(150); // � ������ �������� 150 ���
	}
}

ISR(INT4_vect)
//���������� �� ������� ������ �����
{
	PressButton(UPBUTTON);
}

ISR(INT5_vect)
// ���������� �� ������� ������ ������
{
	PressButton(RIGHTBUTTON);
}

ISR(INT6_vect)
// ���������� �� ������� ������ �����
{
	
	PressButton(LEFTBUTTON);
}

ISR(INT7_vect)
//���������� �� ������� ������ ����
{
	
	PressButton(DOWNBUTTON);
}

int main(void)
// �������� ������� ���������
{
	Init_MK(); // �������������� ��
	LCDinit();  // �������������� �����
	ScreenUpdate(); // ������� ���������� �� �����
    while (1) 
	// ����������� ���� ���������
    {
		DS1307_GetRealTime(); // �������� ����� � �����
		if((timeVar.realHours==timeVar.alarmHours)&&(timeVar.realMinutes==timeVar.alarmMinutes)&&(timeVar.realSeconds>=timeVar.alarmSeconds)&&beep==0)
		// ���� ���� � ������ ������ ����������, � ������� ������ ��� �����, �� ����������� ���������. 
		//����� ���������, ��� ��������� ��� �� ��� �������
		{
			TIMSK=(1<<OCIE0); // �������� ������ 0
		}
		else if((timeVar.realHours!=timeVar.alarmHours)||(timeVar.realMinutes!=timeVar.alarmMinutes))
		// ���� �� ����� �� ����� ����������
		{
			beep=0; // �� ���������� �������� ���
			TIMSK=(0<<OCIE0); // ��������� ���������
		}
		ScreenUpdate(); // ������� ���������� �� �����
		_delay_ms(10); // �������� 10 �� ��� ����� ����������� �����������
    }
}

