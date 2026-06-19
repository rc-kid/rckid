#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/tile_grid.h>
#include <rckid/ui/style.h>

/** Userland uart with pins connected top the cartridge
 
    TODO that as of v3.2 the userland uart *and* the system uart on the debug port are the same. This must be fixed in the next revision.

    TODO would be really nice if this is in some cartridge def file
 */
#define RCKID_USER_UART uart0
#define RCKID_USER_UART_IRQ UART0_IRQ
#define RCKID_USER_UART_TX_PIN 12
#define RCKID_USER_UART_RX_PIN 13


namespace rckid {

    /** A simple serial monitor app. 
     */
    class SerialMonitor : public ui::App<void> {
    public:

        String name() const override { return "SerialMonitor"; }

        SerialMonitor() {
            using namespace ui;
            root_.useBackgroundImage(false);
            root_.setUseHeader(Header::Visibility::OnChange);
            text_ = addChild(new ui::TileGrid{40, 15, Palette256})
                << SetRect(Rect::WH(320, 240));
            tg_ = & text_->contents();
#if (defined PLATFORM_RP2350)
            uart_init(RCKID_USER_UART, baudrate_);
            gpio_set_function(RCKID_USER_UART_TX_PIN, GPIO_FUNC_UART);
            gpio_set_function(RCKID_USER_UART_TX_PIN, GPIO_FUNC_UART);

            // Set up interrupt handler
            irq_set_exclusive_handler(RCKID_USER_UART_IRQ, uartRxIrq);
            irq_set_enabled(RCKID_USER_UART_IRQ, true);

            // Enable UART RX interrupt
            uart_set_irq_enables(RCKID_USER_UART, true, false);
#endif
            tg_->text(0,0) << "Listening at " << baudrate_;
            pos_ = Point{0,1};
            ASSERT(instance_ == nullptr);
            instance_ = this;
        }


        ~SerialMonitor() {
#if (defined PLATFORM_RP2350)
            irq_set_enabled(RCKID_USER_UART_IRQ, false);
            irq_set_exclusive_handler(RCKID_USER_UART_IRQ, nullptr);
            gpio_set_function(RCKID_USER_UART_TX_PIN, GPIO_FUNC_SIO);
            gpio_set_function(RCKID_USER_UART_TX_PIN, GPIO_FUNC_SIO);
#endif
            instance_ = nullptr;
        }

    protected:

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::Start)) {
                auto txt = App::run<TextDialog>();
                if (txt) 
                    txBytes(reinterpret_cast<uint8_t const *>(txt.value().c_str()), txt.value().size());
            }
        }

        void rxBytes() {
#if (defined PLATFORM_RP2350)
            while (uart_is_readable(RCKID_USER_UART)) {
                uint8_t c = uart_getc(RCKID_USER_UART);
                pos_ = tg_->appendChar(pos_, static_cast<char>(c));
            }            
#endif
        }

        void txByte(uint8_t c) {
#if (defined PLATFORM_RP2350)
            uart_putc(RCKID_USER_UART, c);
#endif
            pos_ = tg_->appendChar(pos_, static_cast<char>(c), 16);
        }

        void txBytes(uint8_t const * c, uint32_t numBytes, bool newline = true) {
            while (numBytes > 0) {
                txByte(*c);
                --numBytes;
                ++c;
            }
            if (newline)
                txByte('\n');
        }

        static void uartRxIrq() {
            ASSERT(instance_ != nullptr);
            instance_->rxBytes();
        }

        ui::TileGrid * text_;
        TileGrid * tg_;
        Point pos_;
        uint32_t baudrate_ = 115200;

        static inline SerialMonitor * instance_ = nullptr;

    }; // rckid::SerialMonitor



} // namespace rckid