#ifndef VOICE_CONTROL_H
#define VOICE_CONTROL_H

#include "DFPlayer.h"
#include "driver/uart.h"
#include <string>

class VoiceControl {
public:
  VoiceControl(DFPlayer &player);
  bool begin(uart_port_t uart_num, int tx_io_num, int rx_io_num, int baud = 9600);
  void stop();

private:
  DFPlayer &_player;
  uart_port_t _uart_num;
  int _tx_pin;
  int _rx_pin;
  bool _started;
  void taskLoop();
  static void taskAdapter(void* arg);
  void handleCommand(const std::string &line);
};

#endif
