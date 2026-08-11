#ifndef DFPLAYER_UART_H
#define DFPLAYER_UART_H

#include <cstdint>
#include "driver/uart.h"

class DFPlayer {
public:
  DFPlayer();
  // Inicializa el UART. Devuelve true si la instalación del driver se realizó ok.
  bool begin(uart_port_t uart_num, int tx_io_num, int rx_io_num, int baud = 9600, int rx_buf_size = 1024);

  // Controles básicos
  void play(uint16_t index); // reproducir índice (1..n)
  void next();
  void previous();
  void stop();
  void setVolume(uint8_t vol); // 0..30

  // Opcional: enviar comando genérico
  void sendCommand(uint8_t command, uint8_t feedback, uint8_t paramHigh, uint8_t paramLow);

private:
  uart_port_t _uart_num;
  int _tx_pin;
  int _rx_pin;
  bool _started;

  uint16_t calcChecksum(uint8_t command, uint8_t feedback, uint8_t paramHigh, uint8_t paramLow);
};

#endif // DFPLAYER_UART_H
