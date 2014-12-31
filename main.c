
/*
 * Garageneinfahrt
 *
 *  Created: 11.09.2014
 *  Author: Franz Schneider, Lars Beyer, Thomas Ngandwe
 *  Für das problemlose Compilieren muss dem Linker den Ordner "X:\WinAVR-20100110\avr\lib\avr5" angegeben bekommen
 */

/**MAKROS**/
#define F_CPU 1000000
#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <stdlib.h>             //Bilbiothek für itoa(Integer zu Ascii)
#include <util/delay.h>
#include "lcd-routines.h"
#include <avr/eeprom.h>
#include "uart.h"

/**VARIABLE**/
volatile uint8_t esensor,asensor,best,zeith,zeitm,zeits,alarm1h,alarm1m,alarm2h,alarm2m,poffen,alarmab,anzeigei,i = 0;
//Uhrzeit
char sek[4];
char min[4];
char std[4];
//Öffnungszeit
char al1std[4];
char al1min[4];
//Schließungszeit
char al2std[4];
char al2min[4];

/**Prototypen**/
void menu(void);
void alarm1menu(void);
void alarm2menu(void);
void uitoa(void);
void zeitanzeigen(void);
void alarm1anzeigen(void);
void alarm2anzeigen(void);
void rechnung(void);
void Anzeige(void);
void ladedaten(void);
void schreibedaten(void);
void alarmabfrage(void);
void zeitsenden();
void alarm1senden();
void alarm2senden();
void mc_init();



int main(void)              //Hauptroutine
{

    mc_init();                                  //µC initialisieren
    lcd_init();                                 //LCD initialisieren
    uart_init();                                //UART Schnittstelle initialisieren

    /*Zeit abfragen*/
    {
        uart_puts("\r\n\r\nGerät hochgefahren, warte auf Benutzereingabe.\r\n");
        menu();
        uart_puts("Zeiteingestellt: ");
        zeitsenden();
        uart_puts("\r\n");
        _delay_ms(250);
    }

    /*Alarmdaten abfragen und verarbeiten*/
    {
                alarmabfrage();
            if(alarmab==1)
                {
                    ladedaten();
                    uart_puts("Öffnungs-/Schließungszeiten aus EEPROM geladen!\r\n");
                }
            if(alarmab==0)
                {
                    alarm1menu();
                    uart_puts("Öffnungszeit eingestellt: ");
                    alarm1senden();
                    uart_puts("\r\n");
                    alarm2menu();
                    uart_puts("Schließungszeit eingestellt:!\r\n");
                    alarm2senden();
                    uart_puts("\r\n");
                    schreibedaten();
                    uart_puts("Geraet einsatzbereit!\r\n");
                }
    }

    /*Endlosschleife*/
    while(1)
    {

        uitoa();                                //Integer zu Ascii umwandeln
        Anzeige();
        rechnung();
        //Genaue Wünsche des Kunden müssen noch eingestellt werden
        if(poffen==1)
           {
                PORTD |= (1<<PD6);
                PORTD |= (1<<PD5);
                uart_puts("\r\nEin- unxd Ausfahrt werden geöffnet!");
           }
        if(poffen==0)
        {
                PORTD &= ~(1<<PD6);
                PORTD &= ~(1<<PD5);
                uart_puts("\r\nEin- und Ausfahrt werden geschlossen!");

                if(asensor==1)
                {
                    uart_puts("\r\nAusfahrt wird auf Kundenwunsch geöffnet!");
                    PORTD |= (1<<PD5);
                    _delay_ms(500);
                    PORTD |= (1<<PD5);
                    uart_puts("\r\nAusfahrt wird geschlossen!");
                }

        }
    }
}

ISR(TIMER0_OVF_vect)        //Interrupt - Tastenabfrage
{
	//Tastenabfrage Timer
	TCCR0 |= (1<<CS02)|(1<<CS00);
	TCNT0 =	0xF0;
	if (PIND & (1 << PIND2))        //Eingang
	{
		esensor = 1;
	}
	else
	{
		esensor = 0;
	}
	if (PIND & (1 << PIND3))        //Ausgang
	{
		asensor = 1;
	}
	else
	{
		asensor = 0;
	}
	if (PIND & (1 << PIND4))        //Bestätigung
	{
		best = 1;
	}
	else
	{
		best = 0;
	}
}

ISR(TIMER1_OVF_vect)	    //Interrupt - Zeit hochzählen
{
	TCCR1B |= (1<<CS12)|(1<<CS10);
	TCNT1H = 0xFC;
	TCNT1L = 0x2F;
    zeits++;
    if(zeits>=60)
    {
        zeits=0;
        zeitm++;
        if(zeitm>=60)
        {
            zeitm=0;
            zeith++;
            if(zeith>=24)
                zeith=0;
        }

    }

}

ISR(TIMER2_OVF_vect)	    //Interrupt - Unbenutzt
{
	TCCR2	|= (1<<CS22)|(1<<CS20);
	TCNT2	= 0xFE;


}

void menu()                 //Eingabe Menü
{
    int i = 0;
    while(i != 2)
    {
        uitoa();
        lcd_setcursor(0,1);
        lcd_string("Uhrzeit-Eingabe: ");
        if(i == 0)
            lcd_string("\r\nStunden");
        if(i == 1)
            lcd_string("\r\nMinuten");
        lcd_setcursor(0,2);
        zeitanzeigen();

        if(esensor==1)      //Steigerung der Zeitwerte
        {
            if(i==0)
                zeith++;
            if(i==1)
                zeitm++;
            if(zeith>=24)
                zeith=0;
            if(zeitm>=60)
                zeitm=0;
            _delay_ms(250);
            lcd_clear();
        }

        if(asensor==1)      //Minderung der Zeitwerte
        {
            if(i==0)
            {
                zeith--;
                if (zeith >=24)
                    zeith=23;

            }

            if(i==1)
            {
                zeitm--;
                if(zeitm >= 60)
                    zeitm = 59;
            }

            _delay_ms(250);
            lcd_clear();
        }

        if(best==1)
        {
            i++;
            _delay_ms(250);
            lcd_clear();
        }

    }

}

void zeitanzeigen()         //Zeigt in der gewählten Zeile die aktuelle Zeit an(für LCD)
{
    if(zeith <10)
        {
            lcd_string("0");
        }
        lcd_string(std);
        lcd_string(":");
        if(zeitm <10)
        {
            lcd_string("0");
        }
        lcd_string(min);
        lcd_string(":");
        if(zeits <10)
        {
            lcd_string("0");
        }
        lcd_string(sek);
}

void alarm1anzeigen()       //Zeigt in der gewählten Zeile Öffnungszeit an(für LCD)
{
    if(alarm1h <10)
        {
            lcd_string("0");
        }
        lcd_string(al1std);
        lcd_string(":");
        if(alarm1m <10)
        {
            lcd_string("0");
        }
        lcd_string(al1min);
}

void alarm2anzeigen()       //Zeigt in der gewählten Zeile Schließungszeit an(für LCD)
{
     if(alarm2h <10)
        {
            lcd_string("0");
        }
        lcd_string(al2std);
        lcd_string(":");
        if(alarm2m <10)
        {
            lcd_string("0");
        }
        lcd_string(al2min);

}

void uitoa()                //Integer Zeitwerte in ASCII Umwandeln(für LCD)
{
        /*Uhrzeit */
        itoa( zeits, sek, 10 );
        itoa( zeitm, min, 10 );
        itoa( zeith, std, 10 );
        /*Öffnungszeit */
        itoa( alarm1m, al1min, 10 );
        itoa( alarm1h, al1std, 10 );
        /*Schließungszeit */
        itoa( alarm2m, al2min, 10 );
        itoa( alarm2h, al2std, 10 );

}

void alarm1menu()           //Alarm Öffnungszeit einstellen
{
    int i = 0;
    lcd_clear();
    while(i != 2)
    {
        uitoa();
        lcd_setcursor(0,1);
        lcd_string("Oeffnung: ");
        if(i == 0)
            lcd_string("Stunden");
        if(i == 1)
            lcd_string("Minuten");
        lcd_setcursor(0,2);
        alarm1anzeigen();

        if(esensor==1)      //Steigerung der Zeitwerte
        {
            if(i==0)
                alarm1h++;
            if(i==1)
                alarm1m++;
            if(alarm1h>=24)
                alarm1h=0;
            if(alarm1m>=60)
                alarm1m=0;
            _delay_ms(250);
            lcd_clear();
        }

        if(asensor==1)      //Minderung der Zeitwerte
        {
            if(i==0)
            {
                alarm1h--;
                if (alarm1h >=24)
                    alarm1h=23;
            }

            if(i==1)
            {
                alarm1m--;
                if(alarm1m >= 60)
                    alarm1m = 59;
            }

            _delay_ms(250);
            lcd_clear();
        }

        if(best==1)
        {
            i++;
            _delay_ms(250);
            lcd_clear();
        }

    }

}

void alarm2menu()           //Alarm Schließungszeit einstellen
{
    int i = 0;
    lcd_clear();
    while(i != 2)
    {
        uitoa();

        lcd_setcursor(0,1);
        lcd_string("Schliessung: ");
        if(i == 0)
            lcd_string("Stunden");
        if(i == 1)
            lcd_string("Minuten");
        lcd_setcursor(0,2);
        alarm2anzeigen();

        if(esensor==1)      //Steigerung der Zeitwerte
        {
            if(i==0)
                alarm2h++;
            if(i==1)
                alarm2m++;
            if(alarm2h>=24)
                alarm2h=0;
            if(alarm2m>=60)
                alarm2m=0;
            _delay_ms(250);
            lcd_clear();
        }

        if(asensor==1)      //Minderung der Zeitwerte
        {
            if(i==0)
            {
                alarm2h--;
                if (alarm2h >=24)
                    alarm2h=23;
            }

            if(i==1)
            {
                alarm2m--;
                if(alarm2m >= 60)
                    alarm2m = 59;
            }

            _delay_ms(250);
        }

        if(best==1)
        {
            i++;
            _delay_ms(250);

        }
    }

}

void rechnung(void)         //Alarmrechnung "Ist offen oder zu?"
{
    if(zeith>=alarm1h && zeitm>=alarm1m)
    {
        if(zeith<=alarm2h && zeitm<=alarm2m)
        {
            poffen = 1;                     //für Vergleich in main
        }
    }
    else
    {
            poffen = 0;                     //für Vergleich in main
    }
}

void Anzeige(void)          //LCD Anzeige Uhrzeit/Alarm1/Alarm2 - UART Uhrzeit und ALARM senden
{
        lcd_clear();
        lcd_setcursor(0,1);
        if(anzeigei<=12)     //Uhrzeitanzeigen
        {
            lcd_string("Uhrzeit");
            lcd_setcursor(0,2);
            zeitanzeigen();
        }
        if(alarm1h==alarm2h&&alarm1m==alarm2m)
            {
                            //Genaue Abfrage der Werte - interessanter ist es wenn es nicht stimmt
            }
        else{
            if(anzeigei>15&&anzeigei<=30)
            {
                lcd_string("Oeffnungszeit:");
                lcd_setcursor(0,2);
                alarm1anzeigen();
            }
            if(anzeigei>30&&anzeigei<45)
            {
                lcd_string("Schliessungszeit:");
                lcd_setcursor(0,2);
                alarm2anzeigen();
            }
            if(anzeigei==45)
            {
                anzeigei=0;
                //UART Senden
                i++;

            }
                if(i==0)
                {
                    uart_puts("Uhrzeit: ");
                    zeitsenden();
                    uart_puts("\r\n");
                }
                if(i==1)
                {
                    uart_puts("Öffnungszeit: ");
                    alarm1senden();
                    uart_puts("\r\n");
                }
                if(i==2)
                {
                    uart_puts("Schließungszeit: ");
                    alarm2senden();
                    uart_puts("\r\n");
                }
                if(i==3)i=0;

            anzeigei++;
            }
            _delay_ms(250);
}

void ladedaten(void)        //Alarmdaten aus EEPROM lesen
{
    alarm1h = eeprom_read_byte((uint8_t*)01);
    alarm1m = eeprom_read_byte((uint8_t*)02);
    alarm2h = eeprom_read_byte((uint8_t*)03);
    alarm2m = eeprom_read_byte((uint8_t*)04);
}

void alarmabfrage(void)     //Abfrage ob Daten neu oder aus EEPROM geladen werden soll
{
    int i = 0;
    lcd_clear();
    while(i != 1)
    {
        lcd_setcursor(0,1);
        lcd_string("Int. Werte verwenden?");
        lcd_setcursor(0,2);
        lcd_string("Weiter fuer Best.");

        if(asensor==1)      //Lese neue Werte ein
        {
         alarmab = 0;
         i++;
        }

        if(best==1)
        {
         alarmab = 1;  //Lese Werte aus EEPROM
         i++;
        }
    }
    lcd_clear();

}

void schreibedaten(void)    //Alarm in EEPROM schreiben
{
    eeprom_update_byte((uint8_t*)01, alarm1h);
    eeprom_update_byte((uint8_t*)02, alarm1m);
    eeprom_update_byte((uint8_t*)03, alarm2h);
    eeprom_update_byte((uint8_t*)04, alarm2m);

}

void zeitsenden()           //Gibt die Zeit über UART aus!
{
    if(zeith <10)
        {
            uart_puts("0");
        }
        uart_puts(std);
        uart_puts(":");
        if(zeitm <10)
        {
            uart_puts("0");
        }
        uart_puts(min);
        uart_puts(":");
        if(zeits <10)
        {
            uart_puts("0");
        }
        uart_puts(sek);
        uart_puts("\r\n");
}

void alarm1senden()         //Schicke Öffnungszeit an UART
{
    if(alarm1h <10)
        {
            uart_puts("0");
        }
        uart_puts(al1std);
        uart_puts(":");
        if(alarm1m <10)
        {
            uart_puts("0");
        }
        uart_puts(al1min);
}

void alarm2senden()         //Schicke Schließungszeit an UART
{
     if(alarm2h <10)
        {
            uart_puts("0");
        }
        uart_puts(al2std);
        uart_puts(":");
        if(alarm2m <10)
        {
            uart_puts("0");
        }
        uart_puts(al2min);

}

void mc_init()
{
    //Tastenabfrage Timer
	TCCR0 |= (1<<CS02)|(1<<CS00);           //Vorteiler 1024
	TCNT0 =	0xF0;                           //genauen Teiler setzen

	//Sekunden messen
	TCCR1B |= (1<<CS12)|(1<<CS10);          //Vorteiler 1024
	TCNT1H = 0xFC;                          //7812d
	TCNT1L = 0x2F;                          //
	//Speaker Timer
	TCCR2 |= (1<<CS22)|(1<<CS20);           //Vorteiler 1024
	TCNT2 = 0xFE;                           //genauen Teielr setzen

	TIMSK |= (1<<TOIE0)|(1<<TOIE1)|(1<<TOIE2);  //Maske, welcher Teiler wann gesetzt werden soll.Aufsteigende/Absteigende Flanke
	DDRD = 0b11100000;                          //Input und Output Bits setzen
	sei();                                      //Interrupt Service Routine setzen
}
