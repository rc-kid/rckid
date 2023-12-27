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
        pwm_set_wrap(PWM_SLICE, 256); // set wrap to 8bit sound levels
        
        // set the PWM output to count to 256 441000 times per second
        pwm_set_clkdiv(PWM_SLICE, 11.07);
        // acquire and configure the DMA
        dma_ = dma_claim_unused_channel(true);
        auto dmaConf = dma_channel_get_default_config(dma_);
        channel_config_set_transfer_data_size(& dmaConf, DMA_SIZE_32); // transfer 32 bytes (16 per channel, 2 channels)
        channel_config_set_read_increment(& dmaConf, true);
        //channel_config_set_dreq(& dmaConf, pwm_get_dreq(TIMER_SLICE)); // DMA is driver by the sample rate PWM
        channel_config_set_dreq(&dmaConf, pwm_get_dreq(PWM_SLICE));
        dma_channel_configure(dma_, & dmaConf, &pwm_hw->slice[PWM_SLICE].cc, nullptr, 0, false);
        // enable IRQ0 on the DMA channel (shared with SD card and display)
        dma_channel_set_irq0_enabled(dma_, true);
    }

    void Audio::startPlayback(SampleRate rate, uint16_t * buffer, size_t stereoSamples, CallbackPlay cb) {
        cbPlay_ = cb;
        buffer_ = buffer;
        bufferSize_ = stereoSamples;
        setSampleRate(rate);
        // add shared IRQ handler on the DMA done
        irq_add_shared_handler(DMA_IRQ_0, irqDMADone,  PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
        status_ |= PLAYBACK;
        status_ &= ~ BUFFER_INDEX; // transferring the first half of the buffer
        dma_channel_transfer_from_buffer_now(dma_, buffer, bufferSize_ / 2);
        pwm_set_enabled(PWM_SLICE, true);
        pwm_set_enabled(TIMER_SLICE, true);
    }

    void Audio::stopPlayback() {
        pwm_set_enabled(PWM_SLICE, false);
        pwm_set_enabled(TIMER_SLICE, false);
        irq_remove_handler(DMA_IRQ_0, irqDMADone);
        dma_channel_abort(dma_);
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
                 cbPlay_(buffer_ + ((status_ & BUFFER_INDEX) ? 0 : bufferSize_), bufferSize_ / 2);
            } else if (status_ & RECORDING) {

            }
        }
    }

    void Audio::setSampleRate(SampleRate rate) { setSampleRate(static_cast<uint16_t>(rate)); }

    void Audio::setSampleRate(uint16_t rate) {
        // since even the lower frequency (8kHz) can be obtained with a 250MHz (max) sys clock and 16bit wrap, we keep clkdiv at 1 and only change wrap here
        pwm_set_wrap(TIMER_SLICE, (cpuClockSpeed() * 10 / rate + 5) / 10); 
    }

    void __not_in_flash_func(Audio::irqDMADone)() {
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
        }
    }

} // namespace RCKid