# Interesting Bugs I Met

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