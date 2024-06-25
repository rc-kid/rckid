#include <stdio.h>
#include "pico/stdlib.h"

// use pin 21 for the LED
constexpr uint LED_PIN = 21;

int main() {
  stdio_init_all();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  while (true) {
    gpio_put(LED_PIN, 1);
    sleep_ms(100);
    gpio_put(LED_PIN, 0);
    sleep_ms(100);
  }
}