#include "main.h"

SerialTX serialTX = {.readPos = 0, .writePos = 0};
SerialRX serialRX = {.readPos = 0, .writePos = 0};
volatile uint32_t systemMillis = 0;
volatile uint32_t sampleTimestamp = 0;
volatile uint32_t packetId = 0;
volatile uint16_t valuesADC[2];
volatile uint8_t channelADC = 0;

int main(void) {
    setupTimer();
    // setupPWM();
    setupADC();
    setupUART();
    sei();

    // Variables
    char command[32];
    size_t i = 0;

    while (1) { // Program loop

        if (readStringSerial(command, &i)) {
            if (strcmp(command, "ON") == 0) {
                sbi(PORTB, PORTB0);
            }
            else if (strcmp(command, "OFF") == 0) {
                cbi(PORTB, PORTB0);
            }
        }
    }

    return 0;
}

void setupTimer() {
    TCCR0A = (1 << WGM01); // Set to CTC mode
    OCR0A = 249; // Count from 0 to 249
    TIMSK0 = (1 << OCIE0A); // Enable interrupt
    TCCR0B = (1 << CS01 | (1 << CS00)); // Start at 64 prescalar
}

void setupPWM() {
    TCCR2A = (1 << WGM21) | (1 << WGM20) | (1 << COM2A1); // Use fast-PWM and non-inverting
    TCCR2B = (1 << CS22); // Start at 64 prescaler
    OCR2A = 0; // Set duty cycle (from 0 to 255)
    DDRB |= (1 << PB3); // Output on PB3

    TIMSK2 = (1 << OCIE2B); // Enabled interrupt one timer2
}

void setupADC() {
    ADMUX = (1 << REFS0); // Use Vcc as reference
    ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS1) | (1 << ADPS2); // Enable ADC, ADC interrupt, and prescaler
    DIDR0 = (1 << ADC0D) | (1 << ADC1D); // Disable digital input on both ADC0 and ADC1 to reduce noise
}

void setupUART() {
    UCSR0A = (1 << U2X0); // Set double speed
    UCSR0B = (1 << TXEN0) | (1 << TXCIE0) | (1 << RXEN0) | (1 << RXCIE0); // Enable transmitter/receiver and interrupt
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // Set 8-bit data frame
    UBRR0H = (BRC >> 8);
    UBRR0L = BRC;

    DDRB = (1 << PORTB0); // Set the direction register to PORTB0   
}

void startADC() {
    channelADC = 0; // Reset to ADC0
    ADMUX = (ADMUX & 0xF0) | channelADC; // Force start on ADC0, 0xF0 = 11110000
    ADCSRA |= (1 << ADSC);  // Start conversion
}

// Timer2
ISR(TIMER2_COMPB_vect) {
    static uint8_t duty = 0;
    static int8_t step = 3;

    OCR2A = duty; // Update PWM duty cycle 
    duty += step;

    if (duty == 0 || duty == 255) {
        step = -step; // reverse direction
    }
}

// Timer0
ISR(TIMER0_COMPA_vect) {

    systemMillis++;

    static uint8_t divider = 0;
    divider++;

    // 100 Hz (10 ms) acquisition
    if (divider >= 10) {
        divider = 0;
        sampleTimestamp = systemMillis;
        startADC();
    }
}

// Analog digital converter
ISR(ADC_vect) {

    valuesADC[channelADC] = ADC;

    if (channelADC == 0) {
        channelADC = 1;
        ADMUX = (ADMUX & 0xF0) | 1; // Set to ADC1, 0xF0 = 11110000
        ADCSRA |= (1 << ADSC);
    }
    else {

        channelADC = 0;
        char line[64];

        uint16_t adc0 = (valuesADC[0] * 5000UL) / 1023;
        uint16_t adc1 = (valuesADC[1] * 5000UL) / 1023;

        buildCsvLine(line, packetId++, sampleTimestamp, adc0, adc1);
        writeSerial(line);
    }
}

// UART RX
ISR(USART_RX_vect) {
    
    serialRX.buffer[serialRX.writePos] = UDR0;
    serialRX.writePos++;

    if (serialRX.writePos >= RX_BUFFER_SIZE) {
        serialRX.writePos = 0;
    }
}

// UART TX
ISR(USART_TX_vect) {
    if (serialTX.readPos != serialTX.writePos) {

        UDR0 = serialTX.buffer[serialTX.readPos];
        serialTX.readPos++;

        if (serialTX.readPos >= TX_BUFFER_SIZE) {
            serialTX.readPos = 0;
        }
    }
}

uint8_t readStringSerial(char *command, size_t *i) {

    while (serialRX.readPos != serialRX.writePos) {
        char c = serialRX.buffer[serialRX.readPos];

        serialRX.readPos++;

        if (serialRX.readPos >= RX_BUFFER_SIZE) {
            serialRX.readPos = 0;
        }

        if (c == '\r') {
            continue;
        }        

        if (c == '\n') {
            command[*i] = '\0';
            *i = 0;
            return 1;
        }

        command[*i] = c;
        (*i)++;
    }

    return 0;
}

void writeSerial(char c[]) {

    // Append serial
    for (uint8_t i = 0; i < strlen(c); i++) {
        serialTX.buffer[serialTX.writePos] = c[i];
        serialTX.writePos++;

        if (serialTX.writePos >= TX_BUFFER_SIZE) {
            serialTX.writePos = 0;
        }
    }

    // Transmit serial
    if (UCSR0A & (1 << UDRE0)) { // Wait for transmit buffer to be ready
        UDR0 = serialTX.buffer[serialTX.readPos];
        serialTX.readPos = (serialTX.readPos + 1) % TX_BUFFER_SIZE;
    }
}

void buildCsvLine(char *line, uint32_t packetId, uint32_t ts, uint16_t adc0, uint16_t adc1) {
    uint8_t idx = 0;

    appendUint32(line, &idx, packetId);
    line[idx++] = ',';

    appendUint32(line, &idx, ts);
    line[idx++] = ',';

    appendUint16(line, &idx, adc0);
    line[idx++] = ',';

    appendUint16(line, &idx, adc1);

    line[idx++] = '\n';
    line[idx] = '\0';
}

void appendUint16(char *line, uint8_t *idx, uint16_t value) {
    char tmp[6];
    uint8_t i = 0;

    if (value == 0) {
        line[(*idx)++] = '0';
        return;
    }

    while (value > 0) {
        tmp[i++] = (value % 10) + '0';
        value /= 10;
    }

    while (i > 0) {
        line[(*idx)++] = tmp[--i];
    }
}

void appendUint32(char *line, uint8_t *idx, uint32_t value) {
    char tmp[11];
    uint8_t i = 0;

    if (value == 0) {
        line[(*idx)++] = '0';
        return;
    }

    while (value > 0) {
        tmp[i++] = (value % 10) + '0';
        value /= 10;
    }

    while (i > 0) {
        line[(*idx)++] = tmp[--i];
    }
}

// -----------------------------------------------------------------------
// Not used right now

char peekCharSerial(void) {
    char ret = '\0';

    if (serialRX.readPos != serialRX.writePos) {

        ret = serialRX.buffer[serialRX.readPos];
    }

    return ret;
}

char readCharSerial(void) {
    char ret = '\0';

    if (serialRX.readPos != serialRX.writePos) {
        ret = serialRX.buffer[serialRX.readPos];

        serialRX.readPos++;

        if (serialRX.readPos >= RX_BUFFER_SIZE) {
            serialRX.readPos = 0;
        }
    }

    return ret;
}

void millivoltToCharArray(uint16_t millivolt, char *millivoltBuffer) {
    char buffer[6];

    if (millivolt == 0) {
        millivoltBuffer[0] = '0';
        millivoltBuffer[1] = '\0';
        return;
    }

    uint8_t i = 0;
    while (millivolt > 0) {
        buffer[i++] = (millivolt % 10) + '0';
        millivolt /= 10;
    }

    // Reverse
    for (uint8_t j = 0; j < i; j++) {
        millivoltBuffer[j] = buffer[i - j - 1];
    }

    millivoltBuffer[i] = '\0';
}

void concatenateBufferToLine(char *line, char *buffer0, char *buffer1) {
    
    uint8_t i = 0;
    while (buffer0[i] != '\n' && buffer0[i] != '\0') {
        line[i] = buffer0[i];
        i++;
    }
    line[i++] = ',';

    uint8_t j = 0;
    while (buffer1[j] != '\n' && buffer1[j] != '\0') {
        line[i++] = buffer1[j++];
    }
    
    line[i++] = '\n';
    line[i] = '\0';

    return;
}