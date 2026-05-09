## TODO

- [ ] Add dynamic ADC channels
- [ ] Codify data transfer
- [x] Enable Asynchronus bidirection I/O (add RX with TX)
- [x] Remove float and used int instead (Solve the stuff with 10-bit and 1023.0 ())
- [x] Send multiple values (csv style)
- [x] Add column title
- [x] Make github repo for code I don't want to loose
- [x] Use ISR timer for interrupt


## Compile
make

## Compile and upload
make upload

## Reading
cat /dev/ttyUSB0

## Writing
echo '1' > /dev/ttyUSB0  
echo "ON" > /dev/ttyUSB0
echo "OFF" > /dev/ttyUSB0

## Check serial configuration
stty -F /dev/ttyUSB0 -a

### Ensure that icanon is off, disables canonical mode (processes input immediately).
stty -F /dev/ttyUSB0 9600 cs8 -cstopb -parenb -icanon -echo

### Unsure
sudo chmod 666 /dev/ttyUSB0

## Resources
https://blog.sparkland.ca/adc-serial-atmega328-avr-c/
