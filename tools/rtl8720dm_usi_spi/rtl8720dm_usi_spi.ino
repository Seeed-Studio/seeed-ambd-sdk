/*
 created 27 March 2020
 by Peter Yang <turmary@126.com>
 */

// the device communicates using SPI, so include the library:
#include <SPI.h>

// pins used for the connection with the device
// the other you need are controlled by the SPI library):
#define SPIX             SPI
const int chipSelectPin = SS;

// SPI transfer tags, commonly used by target SPI AT device
enum {
	SPT_TAG_PRE = 0x55,
	SPT_TAG_ACK = 0xBE,
	SPT_TAG_WR  = 0x80,
	SPT_TAG_RD  = 0x00,
	SPT_TAG_DMY = 0xFF, /* dummy */
};

enum {
	SPT_ERR_OK  = 0x00,
	SPT_ERR_DEC_SPC = 0x01,
};

int spi_transfer_cs(uint8_t v) {
  // take the chip select low to select the device
  digitalWrite(chipSelectPin, LOW);

  v = SPIX.transfer(v);

  // take the chip select high to de-select
  digitalWrite(chipSelectPin, HIGH);

  /* wait slave ready to transfer data */
  // delayMicroseconds(50);
  return v;
}

int spi_transfer16_cs(uint16_t v) {
  uint16_t r;

  r  = spi_transfer_cs(v >> 8) << 8;
  r |= spi_transfer_cs(v & 0xFF);
  return r;
}

int at_cmd_write(const uint8_t* buf, uint16_t len, int loop_wait = 50) {
  uint8_t v;
  int i;
  int r = 0;

  spi_transfer16_cs((SPT_TAG_PRE << 8) | SPT_TAG_WR);
  spi_transfer16_cs(len);

  /* wait slave ready to transfer data */
  delayMicroseconds(15000);

  for (i = -1; i < loop_wait; i++) {
    v = spi_transfer_cs(SPT_TAG_DMY);
    if (v == SPT_TAG_ACK) {
      break;
    }
    delayMicroseconds(500);
  }
  if (i >= loop_wait) {
    r = -1; /* timeout */
    goto __ret;
  }

  v = spi_transfer_cs(SPT_TAG_DMY);
  if (v != SPT_ERR_OK && v != SPT_ERR_DEC_SPC) {
    r = -2; /* device not ready */
    goto __ret;
  }

  len = spi_transfer16_cs((SPT_TAG_DMY << 8) | SPT_TAG_DMY);

  /* wait slave ready to transfer data */
  delayMicroseconds(1000);

  for (i = 0; i < len; i++) {
    spi_transfer_cs(buf[i]);
  }
  Serial.print("Trans ");
  Serial.print(len);
  Serial.println("B");
  r = len; /* success transfer len bytes */

__ret:
  return r;
}

int at_cmd_read(uint8_t* buf, uint16_t len, int loop_wait = 50) {
  uint8_t v;
  int i;
  int r = 0;

  spi_transfer16_cs((SPT_TAG_PRE << 8) | SPT_TAG_RD);
  spi_transfer16_cs(len);

  /* wait slave ready to transfer data */
  delayMicroseconds(15000);

  for (i = -1; i < loop_wait; i++) {
    v = spi_transfer_cs(SPT_TAG_DMY);
    if (v == SPT_TAG_ACK) {
      break;
    }
    delayMicroseconds(500);
    Serial.print(v, HEX);
    Serial.print(',');
  }
  if (i >= loop_wait) {
    r = -1; /* timeout */
    goto __ret;
  }

  v = spi_transfer_cs(SPT_TAG_DMY);
  if (v != SPT_ERR_OK && v != SPT_ERR_DEC_SPC) {
    r = -1000 - v; /* device not ready */
    goto __ret;
  }

  len = spi_transfer16_cs((SPT_TAG_DMY << 8) | SPT_TAG_DMY);

  /* wait slave ready to transfer data */
  delayMicroseconds(10000);

  for (i = 0; i < len; i++) {
    buf[i] = spi_transfer_cs(SPT_TAG_DMY);
  }
  r = len; /* success transfer len bytes */

__ret:

  return r;
}

void setup() {
  Serial.begin(115200);
  while(!Serial);

  // start the SPI library:
  SPIX.begin();
  // Start SPI transaction at a quarter of the MAX frequency
  SPIX.beginTransaction(SPISettings(MAX_SPI / 4, MSBFIRST, SPI_MODE0));
  Serial.println("Begin SPI:");

  // initalize the  data ready and chip select pins:
  pinMode(chipSelectPin, OUTPUT);

  Serial.println("\nConnecting");

  // give the slave time to set up
  delay(100);
  Serial.println("Ready! Enter some AT commands");

  return;
}

#define U_BUF_SZ 254
uint8_t u_buf[U_BUF_SZ + 2];
#define S_BUF_SZ 254
uint8_t s_buf[S_BUF_SZ + 2];
int idx = 0;

void loop() {
  uint8_t c;
  int r;

  while (Serial.available()) {
    c = Serial.read();

    /* not empty line or \r\n */
    if (idx == 0) {
      if (c == '\r' || c == '\n') continue;
    }

    /* process all \r, \n, \r\n */
    if (c == '\n') c = '\r';
    u_buf[idx] = c;
    if (idx < U_BUF_SZ)
      idx++;

    /* maybe leave a char '\n' in Serial object RX buffer */
    if (c == '\r')
      break;
  }

  if (idx && u_buf[idx - 1] == '\r') {
    u_buf[idx] = '\0';

    r = at_cmd_write(u_buf, idx);
    if (r < 0) {
      Serial.print("AT_WRITE ERR ");
      Serial.println(r);
    }
    idx = 0;

    delay(500);
  }

  r = at_cmd_read(s_buf, S_BUF_SZ);
  if (r < 0) {
    Serial.print("AT_READ ERR ");
    Serial.println(r);
    delay(500);

  } else if (r >= 0) {
    int i;

    for (i = 0; i < r; i++) {
      char obuf[0x10];

      sprintf(obuf, "R%02X %c", s_buf[i], s_buf[i]);
      Serial.println(obuf);
    }
    // Serial.println("Read OK");
  }
  delay(1000);
  return;
}
