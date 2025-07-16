#include "main.h"


SerialTX serialTX = {.readPos = 0, .writePos = 0};
volatile uint16_t valueADC = 0;
int extraTime = 0;


int main(void)
{
    setupTimer();
    setupUART();
    setupADC();
    sei(); // Magic interrupt

    while (1) 
    {
        // Do something
    }

    return 0;
}

// Timer
ISR(TIMER0_COMPA_vect)
{
    extraTime++;

    if (extraTime > 3) // TODO: Verify what is this number 3
    {
        startADC();
        extraTime = 0;
    }
}

// UART
ISR(USART_TX_vect)
{
    if (serialTX.readPos != serialTX.writePos) 
    {
        UDR0 = serialTX.buffer[serialTX.readPos];
        serialTX.readPos++;
        if (serialTX.readPos >= TX_BUFFER_SIZE)
        {
            serialTX.readPos = 0; // Wrap around if we reach the end of the buffer
        }
    }
}

// Analog digital converter
ISR(ADC_vect)
{
    valueADC = ADC;
    float voltage = (valueADC / 1023.0) * 5.0; // Convert ADC value to voltage
    // float voltage = (adcValue / 1024.0);

    char floatBuffer[12];
    float_to_char_array(voltage, floatBuffer, 6);

    serialWrite(floatBuffer); // Send the string over UART
}


void setupTimer()
{
    TCCR0A = (1 << WGM01); // Set CTC Bit
    OCR0A = 195;
    TIMSK0 = (1 << OCIE0A);
    TCCR0B = (1 << CS02 | (1 << CS00)); // Start at 1024 prescalar
}


void setupUART()
{
    UBRR0H = (BRC >> 8);
    UBRR0L = BRC;
    UCSR0B = (1 << TXEN0) | (1 << TXCIE0); // Enable transmitter and interrupt
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // Set 8-bit data size
}

void setupADC()
{
    ADMUX = (1 << REFS0) | (1 << MUX0); // Use Vcc as reference and channel ADC1 (pin A1)
    ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS1) | (1 << ADPS2); // Enable ADC, ADC interrupt, and prescaler
    DIDR0 = (1 << ADC1D); // Disable digital input on ADC1 to reduce noise
}

void startADC()
{
    ADCSRA |= (1 << ADSC); // Start the conversion
}

void appendSerial(char c)
{
    serialTX.buffer[serialTX.writePos] = c;
    serialTX.writePos++;

    if (serialTX.writePos >= TX_BUFFER_SIZE)
    {
        serialTX.writePos = 0; // Wrap around if buffer is full
    }
}

void serialWrite(char c[])
{
    for (uint8_t i = 0; i < strlen(c); i++)
    {
        appendSerial(c[i]);
    }

    if (UCSR0A & (1 << UDRE0)) // Wait for transmit buffer to be ready
    {
        UDR0 = serialTX.buffer[serialTX.readPos];
        serialTX.readPos = (serialTX.readPos + 1) % TX_BUFFER_SIZE;
    }
}

void float_to_char_array(float num, char *buffer, int precision)
{
    int i = 0;
    int is_negative = 0;

    if (num < 0)
    {
        is_negative = 1;
        num = -num; // Make the number positive
    }

    int int_part = (int)num; // Extract the integer part
    float frac_part = num - int_part;

    // TODO: Convert do while to for loop or while loop
    // Convert the integer part to string
    do
    {
        buffer[i++] = (int_part % 10) + '0'; // Convert digit to character
        int_part /= 10;
    } while (int_part > 0);

    // Add negative sign if needed
    if (is_negative)
    {
        buffer[i++] = '-';
    }

    // Reverse the integer part in the buffer
    for (int j = 0; j < i / 2; j++)
    {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }

    // Add the decimal point
    buffer[i++] = '.';

    // Convert the fractional part to string with the given precision
    for (int j = 0; j < precision; j++)
    {
        frac_part *= 10;
        int digit = (int)frac_part;
        buffer[i++] = digit + '0';
        frac_part -= digit;
    }

    // Add newline and null-terminated characters
    // buffer[i++] = 'V';
    buffer[i++] = '\n';
    buffer[i] = '\0';
}
