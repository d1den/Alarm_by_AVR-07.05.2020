/*
 * ClockApplication.c
 *
 * Created: 03.05.2020 9:52:59
 */ 

#include <avr/io.h> // Подключение стандартной библиотеки микроконтроллеров avr
#define F_CPU 16000000L // Частота тактов CPU равна 16 МГц
#include <util/delay.h> // Подключаем библиотеку задержек
#include <avr/interrupt.h> // Подключение библиотеки прерываний
#include "lcd.h" // Подключение библиотеки для работы с lcd экраном
#include "I2C.h" // Подключение собственной библиотеки для работы с I2C интерфейсом

#define UPBUTTON	INT4_vect_num // Задаём своё имя константе номера вектора прерываниия (по нажатию клавишы вверх)
#define DOWNBUTTON	INT7_vect_num // Аналогично вниз
#define RIGHTBUTTON	INT5_vect_num // Вправо
#define LEFTBUTTON  INT6_vect_num // Влево
#define FIRSTLINE	0 // Номер координат для первой строки цифр (часы)
#define SECONDLINE  6 // Для второй (будильник)
#define ISLOW		1 // Коснтанта для проверки состояния
typedef struct
// Создание структуры, хранящей значения времени часов и будильника 
{
	byte realHours, realMinutes, realSeconds, alarmHours, alarmMinutes, alarmSeconds;
} Time; // Переобредялем, чтобы образаться не struct Time, а сразу Time

byte CursorPosition=0; // Байт, хранящий позицию курсора
byte CursorEnable=0; // Байт, указывающий состояние курсора (для изменения значений) - включено редактирование или нет
// Задаём двумерный массив, хранящий значения координат цифр (с учетом разделителя между ними)
byte CursorPositionMas[12][2]={{6,0},{7,0},{9,0},{10,0},{12,0},{13,0},{6,1},{7,1},{9,1},{10,1},{12,1},{13,1}};
Time timeVar; // Создаём объект структуры, в котором будем хранить значения
unsigned int beep=0; // Переменная, хранящая количество сигналов при срабатывании будильника
void DS1307_GetRealTime() 
// Метод,считывающий реальное время из ds1307 в структуру timeVar
{
	cli(); // Запрещаем прерывания
	// Вызываем метод, записывающий значения времени
	DS1307_GetTime(&(timeVar.realHours),&(timeVar.realMinutes),&(timeVar.realSeconds)); 
	sei(); // Разрешаем прерывания
}

void DS1307_SetTime() 
// Метод, записывающий время в ds1307 из структуры timeVar
{
	cli(); // Запрещаем прерывания
	I2C_WriteRegister(DS1307,HOURS_REGISTER, timeVar.realHours); // Записываем в ds1307 часы
	I2C_WriteRegister(DS1307,MINUTES_REGISTER, timeVar.realMinutes); // Минуты
	I2C_WriteRegister(DS1307,SECONDS_REGISTER, timeVar.realSeconds); // Секунды
	sei(); // Разрешаем прерывания
}

void TwoDigits(byte data)
// Метод, выводящий значение цифры на экран.
// На входе получает 2 цифры в формате BCD, и выводит их на дисплей LCD.
{
	byte high = data>>4;    // получить старшие 4 бита с помощью сдвига вправо
	LCDdata(high+'0');     // отобразить старшую цифру. Прибавляем символ '0' к нашему числу,
	// так как от него идут номера символов цифр
	data &= 0b00001111;           // получить младшие 4 бита
	LCDdata(data+'0');     // отобразить младшую цифру
}

void LCDGotoXYNUM(byte Num)
// Метод, выполняющий переход курсора в координату, заданную массивом координат курсора
{
	LCDGotoXY(CursorPositionMas[Num][0],CursorPositionMas[Num][1]);
}

void ScreenUpdate()
// Метод, выводящий информацию на экран
{
	cli(); // Запрещаем прерывания
	LCDcursorOFF(); // Выключаем курсор
	LCDstringXY("Time=",0,0); // Выводим на первой строке текст 
	LCDGotoXYNUM(FIRSTLINE); // Курсор переходит в координату первого элемента времени 
	TwoDigits(timeVar.realHours); // Отображаем часы
	LCDdata(':'); // Выводим двоеточие
	TwoDigits(timeVar.realMinutes); // Минуты
	LCDdata(':'); // Двоеточие
	TwoDigits(timeVar.realSeconds); // Секунды
	LCDstringXY("Alarm=",0,1); // Выводим на следующей строке текст, что это будильник
	LCDGotoXYNUM(SECONDLINE); // Переходим к первой координате будильника
	TwoDigits(timeVar.alarmHours); // Далее аналогично ыводим время будильника
	LCDdata(':');
	TwoDigits(timeVar.alarmMinutes);
	LCDdata(':');
	TwoDigits(timeVar.alarmSeconds);
	LCDGotoXYNUM(CursorPosition); // Курсор переходит к позиции, которая задана
	if(CursorEnable==1) // Если сейчас режим изменения значений
		LCDcursor_on(); // То включаем мигающий подчеркивающий курсор
	sei(); // Разрешаем прерывания
}

void Init_MK() 
// Первоначальная настройка МК
{
	DDRB=0b00000001; // Настравиваем 0 пин порта b на выход, остальные на вход
	DDRE=0x00; // Настраиваем все пины порта E на вход
	PORTE=0xFF; // Настраиваем все пины порта E на PullUp режим
	EICRB=(1<<ISC71)|(1<<ISC61)|(1<<ISC51)|(1<<ISC41); // Настраиваем внешние прерывания.
	// Устанавливаем необходимые биты в регистре EICRB для 4 прерываний по заднему фронту (с высокого уровня на низкий)
	EIMSK=(1<<INTF7)|(1<<INTF6)|(1<<INTF5)|(1<<INTF4); // Разрешаем 4 внешнийх прерывания INT4-INT7.
	// К данным пинам (PE4-PE7) подключены 4 кнопки, представляющие собой панель управления часами.
	
	// Для работы сигнала будильника настраиваем таймер 0
	TCCR0=(1<<WGM0)|(1<<CS02)|(1<<CS01)|(1<<CS00); // Устанавливаем режим работы CTC (по сравнению) и предделитель 1024
	OCR0=0xff; // Устанавливаем значение значения на сравнение 255
	
	// Для редактирования данных настраиваем таймер 1
	TCCR1B=(1<<WGM12)|(0<<CS12)|(1<<CS11)|(1<<CS10); // Настраиваем также режим CTC и предделитель 64
	OCR1AH=0xff; // Устанавливаем максимальное значение на сравнение = 65 535 в 16 разрядный регистр
	OCR1AL=0xff;
	DS1307_GetRealTime(); // Получаем время с часов
	sei(); // Глобально разрешаем прерывания
}

void EditSeconds(byte *seconds,byte numBut,byte isLow)
// Метод для редактирования секунд
{
	byte high=*seconds>>4; // Получаем старшее значение
	byte low = *seconds & 0b00001111; // И младшее
	if(isLow) // Если мы редактируем младший разряд
	{
		if(numBut==UPBUTTON) // Если кнопка вверх
		low=(low==9)?0:low+1; // То если было число 9, ставим 0, иначе увеличиваем на 1
		if(numBut==DOWNBUTTON) // Если вниз
		low=(low==0)?9:low-1; // ТО если был 0, ставим 9, иначе уменьшаем на 1
	}
	else // Если старший разряд числа
	{
		if(numBut==UPBUTTON) // Если вверх
		high=(high==5)?0:high+1; // то если было 5, ставим 0, иначе +1
		if(numBut==DOWNBUTTON) // Если вниз
		high=(high==0)?5:high-1; // То если был 0, ставим 5, иначе -1
	}
	*seconds=(high<<4)+low; // Возвращаем значение секунд, складывая старший разряд (смещённый влево на 4) и младший
}

void EditMinutes(byte *minutes,byte numBut,byte isLow)
// Метод для редактирования секунд
{
	// Всё аналогично методу для редактирования секунд
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
// Метод для редактирования часов
{
	byte high=*hours>>4; // Старший разряд
	byte low = *hours & 0b00001111; // Младший
	if(isLow) // Если редактируем младший
	{
		if(high==2) // Если старший разряд = 2
		{
			if(numBut==UPBUTTON) // Если нажата вверх
			low=(low==3)?0:low+1; // То если младший = 3, ставим 0, иначе +1
			if(numBut==DOWNBUTTON) // Если вниз
			low=(low==0)?3:low-1; // То если младший = 0, ставим 3, иначе -1
		}
		else // Если старший не 2
		{
			if(numBut==UPBUTTON) // Если вверх
			low=(low==9)?0:low+1; // Если младший = 9, то ставим 0, иначе +1
			if(numBut==DOWNBUTTON) // Если вниз
			low=(low==0)?9:low-1; // То если младший 0, ставим 9, иначе -1
		}
	}
	else // Если же редактируем старший
	{
		if(numBut==UPBUTTON) // Если вверх
		{
			if(high==1) // Если старший разряд = 1
			{
				high=2; // ставим 2
				low=(low>3)?3:low; // Есл младший был > 3, то ставим 3, иначе сотавляем как было 
				// (тк в часах не может отображаться больше 23)
			}
			else if(high==2) // Если же старший разряд 2
			high=0; // То ставим 0
			else // Иначе увеличим на 1
			high++;
			
		}
		if(numBut==DOWNBUTTON) // Если вниз
		{
			if(high==0) // Если 0
			{
				high=2; // Ставим 2
				low=(low>3)?3:low; // Если младший > 3, то ставим 3, иначе оставляем
			}
			else if(high==0) // Если был 0
			high=2; // Устанавливаем 2
			else // Иначе просто уменьшаем
			high--;
		}
	}
	*hours=(high<<4)+low; // Возвращаем часы, состоящие из старшего и младшего разряда
}
void Redact(byte numBut, byte position)
// Метод редактирования выбранного элемента
{
	switch(position)
	// Проверяем, какой элемент редактируем
	{		
		case 0:
			EditHour(&timeVar.realHours,numBut,!ISLOW); // Старший разряд часов
		break;
		case 1:
			EditHour(&timeVar.realHours,numBut,ISLOW); // Младший разряд часов
		break;
		case 2:
			EditMinutes(&timeVar.realMinutes,numBut,!ISLOW); // Аналогично с минутами
		break;
		case 3:
			EditMinutes(&timeVar.realMinutes,numBut,ISLOW);
		break;
		case 4:
			EditSeconds(&timeVar.realSeconds,numBut,!ISLOW); // И с секундами
		break;
		case 5:
			EditSeconds(&timeVar.realSeconds,numBut,ISLOW);
		break;
		case 6:
			EditHour(&timeVar.alarmHours,numBut,!ISLOW); // Далее редактируем время будильника аналогично
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
	DS1307_SetTime(); // Записываем отредактированное время в часы
}

void PressButton(byte NumButton)
// Метод, описывающий действия при нажатии на клавишу
{
	cli(); // Запрещаем прерывания
	TCNT1H=0x00; // Сбрасываем значения таймера 1 для продолжения режима редактирования
	TCNT1L=0x00;
	TIMSK=(0<<OCIE0); // Выключаем таймер 0 (Сигнал будильника при нажатии на любую кнопку)
	sei(); // Разрешаем прерывания
	if(CursorEnable==0) // Если ещё не был включён режим редактирования
	{
		CursorEnable=1; // То включаем его
		CursorPosition=0; // Устанавливаем позицию 0
		cli();
		TIMSK=(1<<OCIE1A); // Включаем таймер времени режима редактирования
		sei();
		LCDGotoXYNUM(CursorPosition); // Переводим курсор в нужную позицию 
	}
	else // Если был ключён
	{
		switch(NumButton)
		// То в зависимости от нажатой кнопки вызываем разные команды
		{
			// Кнопка вверх
			case UPBUTTON:
				Redact(UPBUTTON,CursorPosition); // Редактирование при нажатии кнопки вверх
			break;
			case RIGHTBUTTON: // Если вправо
				if(CursorPosition==11) // Если позиция крайняя, то переходим в 0
					CursorPosition=0;
				else
					CursorPosition++; // Иначе увеличиваем на 1
				LCDGotoXYNUM(CursorPosition); // Переносим курсор
			break;
			case LEFTBUTTON: // Если влево
				if(CursorPosition==0) // Если 0, то позиция 11
					CursorPosition=11;
				else
					CursorPosition--; // Иначе -1
				LCDGotoXYNUM(CursorPosition); //Переносим курсор
			break;
			case DOWNBUTTON: // Если вниз
				Redact(DOWNBUTTON,CursorPosition); // То уменьшаем значение
			break;
		}
	}
}

ISR(TIMER1_COMPA_vect)
// Прерывание по сравнению таймера 1
// Означает, что не было нажатий на кнопки в течение ~3с,
// и режим редактирования выключается
{
	CursorEnable=0; // Выключаем режим редактирования
	LCDcursorOFF(); // Выключаем курсор
	TIMSK=(0<<OCIE1A); // Выключаем таймер 1
}

ISR(TIMER0_COMP_vect)
// Прерывание по сравнению таймера 0.
// Используется для создания переодических звковых сигналов
// и мигания экраном при срабатывании будльника
{
	beep++; // Увеличиваем количество срабатываний таймера
	if(beep%2==0) // Через раз выключаем
		LCDblank();
	else // Или включаем экран
		LCDnblank();
	for(int i=0;i<10;i++) // Даллее с помощью цикла и задержек генерируем звуковой
	// сигнал большой частоты, чтобы был слышен писк (~6,66 кГц)
	{
		if(i%2==0)
			PORTB=0b0000001; // Через раз выдаём на динамик 1
		else // или 0
			PORTB=0;
		_delay_us(150); // И делаем задержку 150 мкс
	}
}

ISR(INT4_vect)
//Прерывание по нажатию кнопки вверх
{
	PressButton(UPBUTTON);
}

ISR(INT5_vect)
// Прерывание по нажатию кнопки вправо
{
	PressButton(RIGHTBUTTON);
}

ISR(INT6_vect)
// Прерывание по нажатию кнопки влево
{
	
	PressButton(LEFTBUTTON);
}

ISR(INT7_vect)
//Прерывание по нажатию кнопки вниз
{
	
	PressButton(DOWNBUTTON);
}

int main(void)
// Основная функция программы
{
	Init_MK(); // Инициализируем МК
	LCDinit();  // Инициализируем экран
	ScreenUpdate(); // Выводим информацию на экран
    while (1) 
	// Бесконечный цикл программы
    {
		DS1307_GetRealTime(); // Получаем время с часов
		if((timeVar.realHours==timeVar.alarmHours)&&(timeVar.realMinutes==timeVar.alarmMinutes)&&(timeVar.realSeconds>=timeVar.alarmSeconds)&&beep==0)
		// Если часы и минуты равный будильнику, а секунды больше или равны, то срабатывает будильник. 
		//Также проверяем, что будильник ещё не был включён
		{
			TIMSK=(1<<OCIE0); // Включаем таймер 0
		}
		else if((timeVar.realHours!=timeVar.alarmHours)||(timeVar.realMinutes!=timeVar.alarmMinutes))
		// Если же время не равно будильнику
		{
			beep=0; // То сбрасываем значение бип
			TIMSK=(0<<OCIE0); // Выключаем будильник
		}
		ScreenUpdate(); // Выводим информацию на экран
		_delay_ms(10); // Задержка 10 мс для более корректного отображения
    }
}

