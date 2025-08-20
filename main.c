    #include "main.h"

    SerialTX serialTX = {.readPos = 0, .writePos = 0};
    volatile uint16_t valuesADC[2];
    volatile uint8_t channelADC = 0;
    uint8_t firstPass = 1;

    int main(void) {
        setupTimer();
        setupADC();
        setupUART();
        sei(); // Magic interrupt

        while (1) {
            // Run program loop
        }

        return 0;
    }

    void setupTimer() {
        TCCR0A = (1 << WGM01); // Set to CTC mode
        OCR0A = 195; // Count from 0 to 195
        TIMSK0 = (1 << OCIE0A); // Enable interrupt
        TCCR0B = (1 << CS02 | (1 << CS00)); // Start at 1024 prescalar
    }

    void setupADC() {
        ADMUX = (1 << REFS0); // Use Vcc as reference
        ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS1) | (1 << ADPS2); // Enable ADC, ADC interrupt, and prescaler
        DIDR0 = (1 << ADC0D) | (1 << ADC1D); // Disable digital input on both ADC0 and ADC1 to reduce noise
    }

    void setupUART() {
        UCSR0B = (1 << TXEN0) | (1 << TXCIE0); // Enable transmitter and interrupt
        UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // Set 8-bit data frame
        UBRR0H = (BRC >> 8);
        UBRR0L = BRC;
    }

    void startADC() {
        channelADC = 0; // Reset to ADC0
        ADMUX = (ADMUX & 0xF0) | channelADC; // Force start on ADC0, 0xF0 = 11110000
        ADCSRA |= (1 << ADSC);  // Start conversion
    }

    // Timer
    ISR(TIMER0_COMPA_vect) {
        startADC();
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
            if (firstPass == 1) {
                writeSerial("\n\n"); // Flush the buffer
                writeSerial("adc0 (mv),adc1 (mv)\n");
                firstPass = 0;
            }

            channelADC = 0;
            char line[32];
            char buffer0[16], buffer1[16];

            uint16_t millivolt0 = (valuesADC[0] * 5000UL) / 1023;
            uint16_t millivolt1 = (valuesADC[1] * 5000UL) / 1023;

            millivoltToCharArray(millivolt0, buffer0);
            millivoltToCharArray(millivolt1, buffer1);
            concatenateBufferToLine(line, buffer0, buffer1); // Put both values on one line

            writeSerial(line);
        }
    }

    // UART
    ISR(USART_TX_vect) {
        if (serialTX.readPos != serialTX.writePos) {

            UDR0 = serialTX.buffer[serialTX.readPos];
            serialTX.readPos++;

            if (serialTX.readPos >= TX_BUFFER_SIZE) {
                serialTX.readPos = 0;
            }
        }
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