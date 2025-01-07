#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#define F_CPU 16000000UL
#define BUAD 9600
#define BRC ((F_CPU / 16 / BUAD) - 1)
#define TX_BUFFER_SIZE 128

#include <util/delay.h>

char serialBuffer[TX_BUFFER_SIZE];
uint8_t serialReadPos = 0;
uint8_t serialWritePos = 0;
volatile uint16_t adcValue = 0;

void float_to_char_array(float num, char *buffer, int precision);
void appendSerial(char c);
void serialWrite(char c[]);

void setupUART(void);
void setupADC(void);
void startADC(void);

int main(void) {

    setupUART();
    setupADC();

    sei();

    while (1) {
       startADC();
        _delay_ms(20);
    }

    return 0;
}

void appendSerial(char c) {
    serialBuffer[serialWritePos] = c;
    serialWritePos++;

    if (serialWritePos >= TX_BUFFER_SIZE) {
        serialWritePos = 0; // Wrap around if buffer is full
    }
}

void serialWrite(char c[]) {
    for (uint8_t i = 0; i < strlen(c); i++) {
        appendSerial(c[i]);
    }

    if (UCSR0A & (1 << UDRE0)) { // If transmit buffer is ready
        UDR0 = serialBuffer[serialReadPos];
        serialReadPos = (serialReadPos + 1) % TX_BUFFER_SIZE;
    }
}

void setupUART() {
    UBRR0H = (BRC >> 8);
    UBRR0L = BRC;
    UCSR0B = (1 << TXEN0) | (1 << TXCIE0); // Enable transmitter and interrupt
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // Set 8-bit data size
}

void setupADC() {
    ADMUX = (1 << REFS0) | (1 << MUX0); // Use Vcc as reference and channel ADC1 (pin A1)
    ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS1) | (1 << ADPS2); // Enable ADC, ADC interrupt, and prescaler
    DIDR0 = (1 << ADC1D); // Disable digital input on ADC1 to reduce noise
}

void startADC() {
    ADCSRA |= (1 << ADSC); // Start the conversion
}

// UART transmit interrupt service routine
ISR(USART_TX_vect) {
    if (serialReadPos != serialWritePos) {
        UDR0 = serialBuffer[serialReadPos];
        serialReadPos++;
        if (serialReadPos >= TX_BUFFER_SIZE) {
            serialReadPos = 0; // Wrap around if we reach the end of the buffer
        }
    }
}

// ADC interrupt service routine
ISR(ADC_vect) {

    adcValue = ADC;
    float voltage = (adcValue / 1023.0) * 5.0; // Convert ADC value to voltage
    // float voltage = (adcValue / 1024.0);

    char floatBuffer[12]; // Sufficient to store the largest 32-bit integer + null terminator
    float_to_char_array(voltage, floatBuffer, 6);

    serialWrite(floatBuffer); // Send the string over UART
}


void float_to_char_array(float num, char *buffer, int precision) {
    int i = 0;
    int is_negative = 0;

    // Handle negative numbers
    if (num < 0) {
        is_negative = 1;
        num = -num; // Make the number positive
    }

    // Extract the integer part
    int int_part = (int)num;
    float frac_part = num - int_part;

    // Convert the integer part to string
    do {
        buffer[i++] = (int_part % 10) + '0'; // Convert digit to character
        int_part /= 10;
    } while (int_part > 0);

    // Add negative sign if needed
    if (is_negative) {
        buffer[i++] = '-';
    }

    // Reverse the integer part in the buffer
    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }

    // Add the decimal point
    buffer[i++] = '.';

    // Convert the fractional part to string with the given precision
    for (int j = 0; j < precision; j++) {
        frac_part *= 10;
        int digit = (int)frac_part;
        buffer[i++] = digit + '0';
        frac_part -= digit;
    }

    // Add '\n' and '\0' at the end
    buffer[i++] = 'V';   // Add newline
    buffer[i++] = '\n';   // Add newline
    buffer[i] = '\0';     // Null-terminate the string
}
