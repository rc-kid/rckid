# Interesting Bugs I Met


## 25/6/2026

In the past, debugging has been pain: The SWD, while really useful, requires an SWD probe(*), which I do not always have with me. I have baked UART over USB into the firmware on RP2350 so that every time the device is connected to a PC and running, a new COM port appears and can be monitored. This is great because **any** computer can now debug the device.

But it comes with its own set of problems - namely every device restart disconnects and reconnects the port and so it is almost impossible to capture the initialization phase. The USB layer is also code heavy so if things break big time (like a hardfault, infinite loop, etc.) the USB device stops working. 

And of course, this does not help with debugging the AVR. 

I have thus designed special cartridges which had GPIOs out that could act as HW UART on RP2350, so with external UART to USB I could monitor the debug messages in a lot more detail. For the AVR I made sure that the AVR IRQ line (which AVR could use to signal to RP2350 HW change) was conveniently the TX pin of AVR's UART connected to RX pin of RP2350's UART so it could double-duty as AVR debug uart viewable by RP2350. Cool, except a lot of stuff happens on the AVR when the RP2350 is off:(

So Beaver King version changed this. I have ditched the AVR IRQ pin entirely (RP2350 polls every frame) and I have used the pins for dedicated AVR UART TX and RP2350 UART TX that go to the debug header on the device directly(2*):

         SWD -- SWD data
         SWC -- SWD clock
         RTX -- RP2350 UART TX for debugging
         ATX -- AVR UART TX for debugging
    UPDI GND -- GND and AVR UPDI pin

This is great! I have readily available very reasonable logging from both chips and I am using it all the time.Debugging has been a breeze with the latest version. **Until I have reached very weird bugs**, to name a few:

- settings cannot be read from the AVR on startup
- brightness cannot be set on startup
- when you press a key, sometimes you get rumble feedback, sometimes not
- when you press a key the lights do not work, sometimes (a key should light up when pressed)
- when you press *some* keys (DPAD specifically), there is a *loong* delay between the rumble and key press

The last manifestation helped me crack this: One difference the DPAD has compared to other buttons is that it is being lit by 4 LEDs, not just 1. So that means 4x the number of I2C messages RP2350 has to send. Maybe the I2C was a culprit.

But the code is essentially the same as the previous version, especially in the sensitive routines (interrupts on both AVR and RP2350). So what could go wrong? Then I realized it: 

The super reliable UART is synchronous. That means that code will not move further *until* the required data is transmitted. I2C is slow, but compared to it, the UART is much slower. Every time I send I2C message to the AVR the AVR will dutifully acknowledge this on the UART lines, even if I am not listening. And this causes a very long delay between receiving the message, and processing the message. And the AVR can only process one message at a time, it sends NACKs when new master write is requested, while operation is still being performed (as the new data would overwrite the previous command in the buffer):

    // master requests to write data itself. ACK if there is no pending I2C message, NACK otherwise. The buffer is reset to 
    } else if ((status & I2C_START_MASK) == I2C_START_RX) {
        if (i2cCommandReady_) {
            TWI0.SCTRLB = TWI_ACKACT_NACK_gc | TWI_SCMD_RESPONSE_gc;
        } else {
            i2cRxBytes_ = 0;
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
        }

As this never happended before because no logging was enabled on AVR most of the time, I never noticed, and I never noticed that my I2C IRQ handler at the RP2350 side did not deal with TX aborts at all. But how to deal with them? 

UART is *so* slow that simply re-triggering the transaction from the IRQ after abort will be enormously wasteful and a *lot* of attempts would be needed to finally bang it through. A standard practice is to implement a backoff strategy where after a failed attempt, we wait a bit and then try again. The amount of time we wait increases with every failed attempt. 

But we can't wait in interrupts! The only thing we can do is to somehow signal from the interrupt to the main loop that a retry is necessary, and let the main loop deal with it. Luckily the I2C already hooks to the `yield()` event that is called periodically (its our way to run sync I2C transactions), so the code changes were pretty small and neat:

    static void __not_in_flash_func(irqHandler)() {
        uint32_t cause = i2c0->hw->intr_stat;
        i2c0->hw->clr_intr;
        // disable the I2C interrupts (otherwise they might fire again immediately despite clearing, especially the TX_EMPTY one)
        i2c0->hw->intr_mask = 0;
        Transaction & t = ring_[ri_];
        // determine if the cause for the interrupt was abort, in which case see if we can retry & backoff accordingly. If no more retries are available, signal an error and if callback was provided, call the callback with -1 bytes received 
        if (cause & I2C_TX_ABORT) {
            if (attempts_++ >= RCKID_I2C_RETRIES) {
                LOG(LL_ERROR, "Unable to perform transaction");
                if (t.callback)
                    t.callback(TransactionError);
                moveToNext();
            } else {
                // to implement the backoff strategy, we must re-trigger outside of the interrupt handler. This is done by setting the retrigger time based current time and linear backoff. This is then checked in the onYield event. Because the retrigger check verifies the retrigger time not to be 0, make sure that if we accidentally landed on it move by 1us
                retrigger_ = TimePoint::now() + attempts_ * RCKID_I2C_BACKOFF_US;
                if (retrigger_ == 0)
                    retrigger_ = retrigger_ + 1;
            }
        // otherwise the transaction was performed correctly (we set the mask according to the transaction type when scheduling)
        } else {
            [...]
        }
    }

    static void onYield() {
        {
            cpu::DisableInterruptsGuard g{};
            if ((retrigger_ != 0) && (TimePoint::now() >= retrigger_)) {
                triggerTransaction();
                return;
            }
            [ bailouts if not sync transaction]
        }
        [ sync transaction handling ]
    }

The code is much more robust now[1]. That said, the lights still do not light up the way I want them to, but at least now no I2C errors and the great loggoing confirms that the messages have been sent & received properly, so I need to look elsewhere. 

(*) Luckily my old Raspberry Pi 3 dev server provides exactly that at home. It consists of the RPi with the GPIOs going out as UART, SWD, I2C and SPI wires that I can use to connect to various interfaces. For details see https://github.com/zduka/devel-server

(2*) I have of course made a mistake and used a pin that is `uart0` TX on RP2350. And this is also the only HW uart that is available on the GPIO cartridge pins, meaning that if a cartridge wants to use UART, I can't use the debugging. Next revision will have pins remapped so that the debug pin will user `uart1`.

[1] Code change in https://github.com/rc-kid/rckid/commit/951c81f5bb98e4f823b5ef69dafceb6dda2c82e5, followed by cleanup & documentation in https://github.com/rc-kid/rckid/commit/c50d153f77c747fba0d11aff8a5889c47765e66f.

## 22/6/2026

I have spent last few days bringing the v3.2 boards to life, with varying degrees of success(*). I was particularly impressed by the software. After I have been developing it in the fantasy console alone for 5 months, it worked out of the box just fine. Except every time I tried to start an application from the launcher, the device froze. Deterministically.

Past lessons remembered I reached for the SWD debugging immediately and found out the culprit quickly - when switching from update mode (where the display just receives data for the viewport forever) to command mode (where the viewport and other settings can be customized), I was disabling the PIO block for 16bit transfer and re-initializing the bitbanging for commands, I did not reinitialize the pins to SIO which resulted in a hard fault. Quick fix:)

I still can't launch apps. This time the device is stuck in endless loop waiting for the display update being done. I have looked at the interrupt handler and noticed that from it I call the callback method that refills the backbuffer with new pixels, while DMA is sending the from buffer. My worry was that the callback takes longer than the DMA transfer in some cases (such as app transitions) and interrupt in interrupt might not work on RP2350, meaning the chain will be broken and update never finished. LLMs agreed with this, so I went on restructuring the interrupt. But nothing changed. I even used up some precious AI credits on this asking for a thorough code review of the display pipeline. It found one after 11pm bug, that luckily could not have contributed (I was not supporting variable buffer sizes for DMA transfers), but still no app launches.

Then I had an idea: if I make the display speed 10x slower, then whatever the callback is doing will finish and no problem there, at least I will have proof that I am on the right path. Except with 10x slower display, the problems start *sooner*. 

So I sit back, relax, and start looking at the code, but crucially this time, I analyze the entire drawing, including the application's render function:

    void RootWidget::render() {
        if (! visible())
            return;
        // update all animations & render essentials
        Widget::renderEssentials();
        // tell the widgets that we are about to render
        onRender();
        // start rendering from rightmost column
        renderCol_ = width() - 1;
        hal::display::update([&](Color::RGB565 * & buffer, uint32_t & bufferSize) {
            if (renderCol_ < 0) {
                buffer = nullptr;
                return;
            } else if (buffer == nullptr) {
                buffer = renderBuffer_.front().data();
                bufferSize = height();
                ASSERT(bufferSize <= renderBuffer_.size());
                renderBuffer_.swap();
            }
            renderColumn(renderCol_--, 0, buffer, height());
        });
    }

    void __not_in_flash_func(irqDMADone)() {
        unsigned irqs = dma_hw->ints0;
        dma_hw->ints0 = irqs;
        if (irqs & (1u << display::dmaChannel)) {
            std::swap(display::buffer, display::backBuffer);
            std::swap(display::bufferSize, display::backBufferSize);
            // if there are pixels to send, restart the DMA immediately
            if (display::buffer != nullptr) {
                dma_channel_set_read_addr(display::dmaChannel, display::buffer, false);
                dma_channel_set_transfer_count(display::dmaChannel, display::bufferSize, true);
            }
            if (display::pixelsToWrite > 0) {
                display::cb(display::backBuffer, display::backBufferSize);
                display::pixelsToWrite -= display::bufferSize;
            } else {
                display::backBuffer = nullptr;
                display::backBufferSize = 0;
            }
        }
        [...]
    }

And luckily, the error now screams in my face:) Note how the interrupt function stops the restarts if the buffer is `nullptr`. Also note how the render routine sets the buffer to null as a precaution if rendered column is below 0 (clearly - no renderable data beyond that). The display update routine is smart and it waits until the previous frame is done before starting the new one. But the `renderCol_` belogs to the root widget and is set *before* the update is called, making the old frame decrement some of the cols (while rendering wrong ones), only to be stopped by the interrupt's own setting of the buffer to `nullptr` when the pixels to write (controlled by the driver) reaches 0. Then the new frame will start updating, but its `renderCol_` is now smaller than expected, thus will render below 0, and set the buffers to `nullptr` without ever clearing all the pixels, being stuck in infinite loop waiting for the update done without any update actually progressing.

So TL;DR: A subtle data race and two "competing" systems for determining when to stop frame update caused this problem. A fix was easy, and actually helped the code in terms of robustness and clarity as well[1].

(*) Most things work as intended. Black PCBs look great, battery connector is awesome. The new headphone detection circuit is probably not working, and not recoverable in SW. 
[1] https://github.com/rc-kid/rckid/commit/8abcdb6a27a1878940181fe24421e726923d9f64