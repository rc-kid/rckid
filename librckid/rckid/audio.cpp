#include "hardware/pwm.h"
#include "hardware/dma.h"

#include "rckid.h"

#include "audio.h"

namespace rckid {

    void Audio::initialize() {
        // configure the PWM pins
        gpio_set_function(RP_PIN_PWM_RIGHT, GPIO_FUNC_PWM);
        gpio_set_function(RP_PIN_PWM_LEFT, GPIO_FUNC_PWM);
        // configure the audio out PWM
        //pwm_set_wrap(PWM_SLICE, 254); // set wrap to 8bit sound levels
        pwm_set_wrap(PWM_SLICE, 4096); // set wrap to 8bit sound levels
        
        pwm_set_clkdiv(PWM_SLICE, 1.38); // 12bit depth @ 44.1kHz and 250MHz
        // set the PWM output to count to 256 441000 times per second
        //pwm_set_clkdiv(PWM_SLICE, 11.07);
        //pwm_set_clkdiv(PWM_SLICE, 61.03);
        // acquire and configure the DMA
        dma0_ = dma_claim_unused_channel(true);
        dma1_ = dma_claim_unused_channel(true);
        /*
        auto dmaConf = dma_channel_get_default_config(dma_);
        channel_config_set_transfer_data_size(& dmaConf, DMA_SIZE_32); // transfer 32 bytes (16 per channel, 2 channels)
        channel_config_set_read_increment(& dmaConf, true);
        channel_config_set_dreq(& dmaConf, pwm_get_dreq(TIMER_SLICE)); // DMA is driver by the sample rate PWM
        //channel_config_set_dreq(&dmaConf, pwm_get_dreq(PWM_SLICE));
        dma_channel_configure(dma_, & dmaConf, &pwm_hw->slice[PWM_SLICE].cc, nullptr, 0, false);
        // enable IRQ0 on the DMA channel (shared with SD card and display)
        dma_channel_set_irq0_enabled(dma_, true);
        */
    }

    void Audio::startPlayback(SampleRate rate, uint16_t * buffer, size_t bufferSize, CallbackPlay cb) {
        configureDMA(dma0_, dma1_, buffer, bufferSize / 2); 
        configureDMA(dma1_, dma0_, buffer + bufferSize / 2, bufferSize / 2);
        cbPlay_ = cb;
        buffer_ = buffer;
        bufferSize_ = bufferSize;
        setSampleRate(rate);
        // add shared IRQ handler on the DMA done
        irq_add_shared_handler(DMA_IRQ_0, irqDMADone,  PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
        status_ |= PLAYBACK;
        status_ &= ~ BUFFER_INDEX; // transferring the first half of the buffer
        dma_channel_start(dma0_);
        //dma_channel_transfer_from_buffer_now(dma0_, buffer, bufferSize_ / 2);
        pwm_set_enabled(PWM_SLICE, true);
        //pwm_set_enabled(TIMER_SLICE, true);
    }

    void Audio::stopPlayback() {
        pwm_set_enabled(PWM_SLICE, false);
        pwm_set_enabled(TIMER_SLICE, false);
        irq_remove_handler(DMA_IRQ_0, irqDMADone);
        dma_channel_abort(dma0_);
        dma_channel_abort(dma1_);
        status_ &= ~PLAYBACK;
    }

    void Audio::startRecording(SampleRate rate) {

    }

    void Audio::stopRecording() {

    }

    void Audio::processEvents() {
        if (status_ & CALLBACK) {
            status_ &= ~CALLBACK;
            if (status_ & PLAYBACK) {
                cbPlay_(buffer_ + ((status_ & BUFFER_INDEX) ? 0 : bufferSize_ / 2), bufferSize_ / 2);
            } else if (status_ & RECORDING) {
                UNIMPLEMENTED; 
            }
        }
    }

    void Audio::configureDMA(int dma, int other, uint16_t const * buffer, size_t bufferSize) {
        auto dmaConf = dma_channel_get_default_config(dma);
        channel_config_set_transfer_data_size(& dmaConf, DMA_SIZE_32); // transfer 32 bytes (16 per channel, 2 channels)
        channel_config_set_read_increment(& dmaConf, true);
        //channel_config_set_dreq(& dmaConf, pwm_get_dreq(TIMER_SLICE)); // DMA is driver by the sample rate PWM
        channel_config_set_dreq(&dmaConf, pwm_get_dreq(PWM_SLICE));
        channel_config_set_chain_to(& dmaConf, other); // chain to the other channel
        dma_channel_configure(dma, & dmaConf, &pwm_hw->slice[PWM_SLICE].cc, buffer, bufferSize / 2, false); // the buffer consists of stereo samples, (32bits), i.e. buffer size / 2
        // enable IRQ0 on the DMA channel (shared with SD card and display)
        dma_channel_set_irq0_enabled(dma, true);
    }

    void Audio::setSampleRate(SampleRate rate) { setSampleRate(static_cast<uint16_t>(rate)); }

    void Audio::setSampleRate(uint16_t rate) {
        // since even the lower frequency (8kHz) can be obtained with a 250MHz (max) sys clock and 16bit wrap, we keep clkdiv at 1 and only change wrap here
        pwm_set_wrap(TIMER_SLICE, (cpuClockSpeed() * 10 / rate + 5) / 10); 
    }

    void __not_in_flash_func(Audio::irqDMADone)() {
        if (dma_channel_get_irq0_status(dma0_)) {
            dma_channel_acknowledge_irq0(dma0_); // clear the flag
            // update dma0 address 
            dma_channel_set_read_addr(dma0_, buffer_, false);
            // we finished sending first part of buffer and are now sending the second part
            status_ &= ~BUFFER_INDEX;
            status_ |= CALLBACK;
        } else if (dma_channel_get_irq0_status(dma1_)) {
            dma_channel_acknowledge_irq0(dma1_); // clear the flag
            // update dma1 address 
            dma_channel_set_read_addr(dma1_, buffer_ + bufferSize_ / 2, false);
            // and set the callback appropriately
            status_ |= BUFFER_INDEX | CALLBACK;
        }
        /*
        if(dma_channel_get_irq0_status(dma_)) {
            dma_channel_acknowledge_irq0(dma_); // clear the flag
            if (status_ & PLAYBACK) {
                status_ |= CALLBACK;
                if (status_ & BUFFER_INDEX) {
                    // transfer first half of the buffer
                    status_ &= ~ BUFFER_INDEX;
                    dma_channel_transfer_from_buffer_now(dma_, buffer_, bufferSize_ / 2);
                } else {
                    // transfer the second half of the buffer
                    status_ |= BUFFER_INDEX;
                    // we add 16bit integers, but copy 32bit numbers (stereo)
                    dma_channel_transfer_from_buffer_now(dma_, buffer_ + bufferSize_, bufferSize_ / 2);
                }
            } else if (status_ & RECORDING) {
                status_ |= CALLBACK;
            }
        } */
    }

} // namespace RCKid