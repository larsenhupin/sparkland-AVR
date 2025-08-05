## TODO

- [ ] Add dynamic ADC channels
- [ ] Enable Asynchronus bidirection I/O (add RX with TX)
- [ ] Codify data transfer
- [X] Remove float and used int instead (Solve the stuff with 10-bit and 1023.0 ())
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
echo '0' > /dev/ttyUSB0

## Resources
file:///Data/Project/Atmega/Atmega328p_datasheet.pdf

### Ensure that icanon is off, disables canonical mode (processes input immediately).

stty -F /dev/ttyUSB0 9600 cs8 -cstopb -parenb -icanon -echo

### Unsure

sudo chmod 666 /dev/ttyUSB0
