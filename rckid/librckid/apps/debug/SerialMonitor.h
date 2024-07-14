#pragma once

#include "rckid/app.h"

namespace rckid {

    class SerialMonitor : public App<FrameBuffer<ColorRGB>> {
    public:
        static SerialMonitor * create() { return new SerialMonitor{}; }

    protected:

        void onFocus() override {
            App::onFocus();
            // enable the serial port
            uart_init(uart0, 115200);
            gpio_set_function(16, GPIO_FUNC_UART);
            gpio_set_function(17, GPIO_FUNC_UART);
            uart_set_hw_flow(uart0, false, false); // disable CTS/RST
            irq_set_exclusive_handler(UART0_IRQ, onRx);
            irq_set_enabled(UART0_IRQ, true);

            // Now enable the UART to send interrupts - RX only
            uart_set_irq_enables(uart0, true, false);            

        }

        void onBlur() override {
            App::onBlur();
            irq_set_enabled(UART0_IRQ, false);
        } 

        void update() override {
            App::update();
        }

        void draw() override {
        }

        static void onRx() {
            while (uart_is_readable(uart0)) {
                uint8_t ch = uart_getc(uart0);
                LOG((char)ch);
                /*
                // Can we send it back?
                if (uart_is_writable(UART_ID)) {
                    // Change it slightly first!
                    ch++;
                    uart_putc(UART_ID, ch);
                }
                chars_rxed++;
                */
            }            
        }

    }; // rckid::SerialMonitor

} // namespace rckid