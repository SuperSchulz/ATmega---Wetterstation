#ifndef UART_H_INCLUDED
#define UART_H_INCLUDED

void uart_init();
int uart_putc(unsigned char );
void uart_puts (char *);

void uart_init()
{
/* USART-Init 9600 Baud bei 8MHz für Mega16 */
// Baudratenregister 8 MHz 9600Bps
	UBRRH = 0; // Highbyte 0
	UBRRL = 12; // Lowbyte  51 ( dezimal)
// UART TX einschalten
	UCSRB |= 0b00001000;
	//UCSRB |= ( 1 << TXEN );
// Asynchron 8N1
	UCSRC =0b10000110;
	//UCSRC |= ( 1 << URSEL )|( 1<<UCSZ0 )|(1<<UCSZ1);
}


/* ATmega16 */
int uart_putc(unsigned char c)
{
    while (!(UCSRA & (1<<UDRE)))  /* warten bis Senden moeglich */
    {
    }

    UDR = c;                      /* sende Zeichen */
    return 0;
}


/* puts ist unabhaengig vom Controllertyp */
void uart_puts (char *s)
{
    while (*s)
    {   /* so lange *s != '\0' also ungleich dem "String-Endezeichen" */
        uart_putc(*s);
        s++;
    }
}


#endif // UART_H_INCLUDED
