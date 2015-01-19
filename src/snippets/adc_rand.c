
//return a random bit
#define rand1_adc() (adcrand_read(ADC_RAND) & 0x01)

//return a random 8 bit
uint8_t rand8_adc(void) {
  uint8_t tmp=0;
  uint8_t mask = 0x80;  //msb first

  do {
    tmp |= (rand1_adc)?mask:0;  //set / clear random bits
    mask = mask >> 1;  //shift to the next bit
  } while (mask);  //until all bits are done
  return tmp;
}

//return a random 16-bit
uint16_t rand16_adc(void) {
  uint16_t tmp;
  tmp = rand8_adc(); //obtain the msb
  tmp = tmp << 8;
  tmp|= rand8_adc(); //obtain the lsb
  return tmp;
}

//return a random 32-bit
uint32_t rand32_adc(void) {
  uint32_t tmp;
  tmp = rand16_adc(); //obtain the msw
  tmp = tmp << 16;
  tmp|= rand16_adc(); //obtain the lsb
  return tmp;
}
