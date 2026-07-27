#include <hardware/dma.h>

#include <rckid/buffer.h>
#include <rckid/capabilities/jacdac.h>

#include <sws_rx.pio.h>
#include <sws_tx.pio.h>


#define JACDAC_BAUDRATE 1000000

/** By default we use pio1, which is set to service the lower pins.
 */
#define JACDAC_PIO pio1

#define JACDAC_PIN 19

namespace rckid {

    struct JacdacImpl {
        Jacdac jacdac;
        int sm;
        unsigned txOffset;
        unsigned rxOffset;
        int dma; 
        volatile bool rxReady = false;
        // double buffer for receiving messages, large enough to store two frames
        DoubleBuffer<uint8_t> rxBuffer{sizeof(jacdac::Frame)};
    }; // JacdacImpl

    namespace {
        JacdacImpl * instance_;

        void __not_in_flash_func(jacdacRxDone)() {
            pio_interrupt_clear(JACDAC_PIO, 0);
            // by the time we get here we can assume that the DMA has transferred everything we need, no need to wait for anything. Store the size of the received frame in the back buffer (transfer count decrements from the initial value, which we know was set to frame size)
            instance_->rxBuffer.back().setUsed(sizeof(jacdac::Frame) - dma_hw->ch[instance_->dma].transfer_count);
            // swap front & back buffers and set the rxReady flag (only if the front buffer is already processed)
            if (instance_->rxReady == false) {
                instance_->rxBuffer.swap();
                instance_->rxReady = true;
            } else {
                // have t drop the back buffer
            }
            // restart the DMA to receive the next frame into the back buffer
            dma_channel_transfer_to_buffer_now(instance_->dma, instance_->rxBuffer.back().data(), sizeof(jacdac::Frame));
            // TODO process the frame here perhaps? 
        }

    }; // anonynous namespace


    Jacdac * Jacdac::instance() {
        if (instance_ == nullptr)
            instance_ = new JacdacImpl();
        return & (instance_->jacdac);
    }

    void Jacdac::onTick() {
        UNREACHABLE; // this should never be called, all the functionality is in the JacdacImpl class
    }



    void Jacdac::doEnable() {
        // claim sm and load programs, do not initialize the programs yet - we do that when we want to use them
        instance_->sm = pio_claim_unused_sm(JACDAC_PIO, true);
        instance_->txOffset = pio_add_program(JACDAC_PIO, &sws_tx_program);
        instance_->rxOffset = pio_add_program(JACDAC_PIO, &sws_rx_program);
        // get dma channel
        instance_->dma = dma_claim_unused_channel(true);
    }

    void Jacdac::doDisable() {

        pio_sm_set_enabled(JACDAC_PIO, instance_->sm, false);
        pio_remove_program(JACDAC_PIO, &sws_tx_program, instance_->txOffset);
        pio_remove_program(JACDAC_PIO, &sws_rx_program, instance_->rxOffset);
        pio_sm_unclaim(JACDAC_PIO, instance_->sm);
        dma_channel_unclaim(instance_->dma);

    }


    bool Jacdac::doSend(uint8_t const * data, uint32_t numBytes) {
        // stop any ongoing transfers
        pio_sm_set_enabled(JACDAC_PIO, instance_->sm, false);
        dma_channel_abort(instance_->dma);
        // wait for the pio to be idle, i.e. high
        // TODO this is wrong, the bus is idle when high consecutively for larger period of time
        gpio::setAsInput(JACDAC_PIN);
        while (gpio::read(JACDAC_PIN) == false)
            ;
        // send the packet start command
        gpio::outputLow(JACDAC_PIN);
        cpu::delayUs(12);
        // wait for 49us so that we can start sending the data TODO why 49us?
        gpio::setAsInput(JACDAC_PIN);
        cpu::delayUs(49);
        // initialize the DMA to send the data to the PIO
        dma_channel_config c = dma_channel_get_default_config(instance_->dma);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
        channel_config_set_dreq(&c, pio_get_dreq(JACDAC_PIO, instance_->sm, true));
        channel_config_set_read_increment(&c, true);
        channel_config_set_write_increment(&c, false);
        // write addressis the TX FIFO, read address is null here, set by transfer from buffer below explicitly
        dma_channel_configure(instance_->dma, &c, &JACDAC_PIO->txf[instance_->sm], nullptr, 0, false);
        dma_channel_transfer_from_buffer_now(instance_->dma, data, numBytes);
        // start the PIO
        sws_tx_program_init(JACDAC_PIO, instance_->sm, instance_->txOffset, JACDAC_PIN);
        pio_sm_set_clock_speed(JACDAC_PIO, instance_->sm, 8 * JACDAC_BAUDRATE);
        pio_sm_set_enabled(JACDAC_PIO, instance_->sm, true);
        // wait for the DMA to finish and the PIO to be idle
        while (! pio_sm_is_stalled(JACDAC_PIO, instance_->sm))
            ;
        pio_sm_set_enabled(JACDAC_PIO, instance_->sm, false);
        // send the final BRK
        gpio::outputHigh(JACDAC_PIN);
        cpu::delayUs(1);
        gpio::low(JACDAC_PIN);
        cpu::delayUs(12);
        gpio::setAsInput(JACDAC_PIN);
        // TODO enable rx again

        return true;
    }

    void Jacdac::doReceive() {
        // stop any ongoing transfers
        pio_sm_set_enabled(JACDAC_PIO, instance_->sm, false);
        dma_channel_abort(instance_->dma);
        // we expect we are not sending anything at the moment, load the PIO sws receiver, configure the DMA and start the PIO. 
        gpio::setAsInput(JACDAC_PIN);
        // configure the DMA
        dma_channel_config c = dma_channel_get_default_config(instance_->dma);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
        channel_config_set_dreq(&c, pio_get_dreq(JACDAC_PIO, instance_->sm, false));
        channel_config_set_read_increment(&c, false);
        channel_config_set_write_increment(&c, true);
        // write address is nullptr (will be set by transfer to buffer below), read address is the RX FIFO of the pio
        dma_channel_configure(instance_->dma, &c, nullptr, &JACDAC_PIO->rxf[instance_->sm], sizeof(jacdac::Frame), false);
        // load & configure the PIO program
        sws_rx_program_init(JACDAC_PIO, instance_->sm, instance_->rxOffset, JACDAC_PIN);
        pio_sm_set_clock_speed(JACDAC_PIO, instance_->sm, 8 * JACDAC_BAUDRATE);
        // enable the interrupt from the pio, which is fired when the end BRK is detected
        irq_set_exclusive_handler(PIO1_IRQ_0, jacdacRxDone);
        irq_set_enabled(PIO1_IRQ_0, true);
        // enable the PIO & DMA
        pio_sm_set_enabled(JACDAC_PIO, instance_->sm, true);
        dma_channel_transfer_to_buffer_now(instance_->dma, instance_->rxBuffer.back().data(), sizeof(jacdac::Frame));
    }

} // namespace rckid