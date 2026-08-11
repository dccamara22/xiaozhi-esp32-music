#include "DFPlayer.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <cstring>

static const char *TAG = "DFPlayer";

DFPlayer::DFPlayer()
  : _uart_num(UART_NUM_1), _tx_pin(-1), _rx_pin(-1), _started(false) {}

bool DFPlayer::begin(uart_port_t uart_num, int tx_io_num, int rx_io_num, int baud, int rx_buf_size) {
  _uart_num = uart_num;
  _tx_pin = tx_io_num;
  _rx_pin = rx_io_num;

  uart_config_t uart_config = {};
  uart_config.baud_rate = baud;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity    = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_1;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.source_clk = UART_SCLK_APB;

  esp_err_t res = uart_param_config(_uart_num, &uart_config);
  if (res != ESP_OK) {
    ESP_LOGE(TAG, "uart_param_config failed: %d", res);
    return false;
  }

  res = uart_set_pin(_uart_num, _tx_pin, _rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  if (res != ESP_OK) {
    ESP_LOGE(TAG, "uart_set_pin failed: %d", res);
    return false;
  }

  res = uart_driver_install(_uart_num, rx_buf_size, 0, 0, nullptr, 0);
  if (res != ESP_OK) {
    ESP_LOGE(TAG, "uart_driver_install failed: %d", res);
    return false;
  }

  _started = true;
  ESP_LOGI(TAG, "DFPlayer UART initialized (uart=%d, tx=%d, rx=%d, baud=%d)", _uart_num, _tx_pin, _rx_pin, baud);
  return true;
}

uint16_t DFPlayer::calcChecksum(uint8_t command, uint8_t feedback, uint8_t paramHigh, uint8_t paramLow) {
  int sum = 0xFF + 0x06 + command + feedback + paramHigh + paramLow;
  int chk = 0 - sum;
  return static_cast<uint16_t>(chk & 0xFFFF);
}

void DFPlayer::sendCommand(uint8_t command, uint8_t feedback, uint8_t paramHigh, uint8_t paramLow) {
  if (!_started) {
    ESP_LOGW(TAG, "DFPlayer not started; call begin() first");
    return;
  }

  uint8_t packet[10];
  packet[0] = 0x7E;
  packet[1] = 0xFF;
  packet[2] = 0x06;
  packet[3] = command;
  packet[4] = feedback;
  packet[5] = paramHigh;
  packet[6] = paramLow;

  uint16_t chk = calcChecksum(command, feedback, paramHigh, paramLow);
  packet[7] = (chk >> 8) & 0xFF;
  packet[8] = chk & 0xFF;
  packet[9] = 0xEF;

  int written = uart_write_bytes(_uart_num, reinterpret_cast<const char*>(packet), sizeof(packet));
  if (written != (int)sizeof(packet)) {
    ESP_LOGW(TAG, "uart_write_bytes wrote %d/%d bytes", written, (int)sizeof(packet));
  } else {
    ESP_LOGD(TAG, "Sent DFPlayer command 0x%02X (%d bytes)", command, written);
  }
}

void DFPlayer::play(uint16_t index) {
  uint8_t high = (index >> 8) & 0xFF;
  uint8_t low  = index & 0xFF;
  sendCommand(0x03, 0x00, high, low);
}

void DFPlayer::next() {
  sendCommand(0x01, 0x00, 0x00, 0x00);
}

void DFPlayer::previous() {
  sendCommand(0x02, 0x00, 0x00, 0x00);
}

void DFPlayer::stop() {
  sendCommand(0x16, 0x00, 0x00, 0x00);
}

void DFPlayer::setVolume(uint8_t vol) {
  if (vol > 30) vol = 30;
  sendCommand(0x06, 0x00, 0x00, vol);
}
