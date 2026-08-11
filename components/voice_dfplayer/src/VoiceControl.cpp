#include "VoiceControl.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <algorithm>

static const char *TAG = "VoiceControl";

VoiceControl::VoiceControl(DFPlayer &player)
  : _player(player), _uart_num(UART_NUM_1), _tx_pin(-1), _rx_pin(-1), _started(false) {}

bool VoiceControl::begin(uart_port_t uart_num, int tx_io_num, int rx_io_num, int baud) {
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

  res = uart_driver_install(_uart_num, 1024, 0, 0, nullptr, 0);
  if (res != ESP_OK) {
    ESP_LOGE(TAG, "uart_driver_install failed: %d", res);
    return false;
  }

  _started = true;
  xTaskCreate(&VoiceControl::taskAdapter, "voice_ctrl", 4096, this, 5, nullptr);
  ESP_LOGI(TAG, "VoiceControl started (uart=%d, tx=%d, rx=%d, baud=%d)", _uart_num, _tx_pin, _rx_pin, baud);
  return true;
}

void VoiceControl::stop() {
  _started = false;
  // Nota: no se elimina driver/cola aquí; se puede mejorar si es necesario.
}

void VoiceControl::taskAdapter(void* arg) {
  static_cast<VoiceControl*>(arg)->taskLoop();
  vTaskDelete(nullptr);
}

void VoiceControl::taskLoop() {
  std::string buf;
  const int BUF_SIZE = 256;
  uint8_t data[BUF_SIZE];

  while (_started) {
    int len = uart_read_bytes(_uart_num, data, BUF_SIZE, pdMS_TO_TICKS(100));
    if (len > 0) {
      for (int i = 0; i < len; ++i) {
        char c = static_cast<char>(data[i]);
        if (c == '\n' || c == '\r') {
          if (!buf.empty()) {
            handleCommand(buf);
            buf.clear();
          }
        } else {
          buf.push_back(c);
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void VoiceControl::handleCommand(const std::string &line) {
  std::string s = line;
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);

  if (s.rfind("play", 0) == 0) {
    size_t pos = s.find_first_not_of(" ", 4);
    int n = 1;
    if (pos != std::string::npos) {
      try { n = std::stoi(s.substr(pos)); } catch(...) { n = 1; }
    }
    ESP_LOGI(TAG, "CMD: play %d", n);
    _player.play((uint16_t)n);
  } else if (s.find("next") != std::string::npos) {
    ESP_LOGI(TAG, "CMD: next");
    _player.next();
  } else if (s.find("prev") != std::string::npos || s.find("previous") != std::string::npos) {
    ESP_LOGI(TAG, "CMD: previous");
    _player.previous();
  } else if (s.find("stop") != std::string::npos) {
    ESP_LOGI(TAG, "CMD: stop");
    _player.stop();
  } else if (s.find("volume") != std::string::npos) {
    size_t pos = s.find_last_of(' ');
    int v = 20;
    if (pos != std::string::npos) {
      try { v = std::stoi(s.substr(pos + 1)); } catch(...) { v = 20; }
    }
    if (v < 0) v = 0;
    if (v > 30) v = 30;
    ESP_LOGI(TAG, "CMD: volume %d", v);
    _player.setVolume((uint8_t)v);
  }
}
