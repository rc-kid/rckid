#include <hardware/dma.h>

#include <rckid/capabilities/jacdac.h>

#include <sws_rx.pio.h>
#include <sws_tx.pio.h>


#define JACDAC_BAUDRATE 1000000

/** By default we use pio1, which is set to service the lower pins.
 */
#define JACDAC_PIO pio1

#define JACDAC_PIN 19


namespace rckid {

    namespace {
        Jacdac instance_;
        int sm_;
        unsigned txOffset_;
        unsigned rxOffset_;
        int dma_; 


        void __not_in_flash_func(jacdacRxDone)() {
            pio_interrupt_clear(JACDAC_PIO, 0);
            // TODO we have completed the transfer (assuming it was a valid one, check the frame)
        }

    }; // anonynous namespace


    Jacdac * Jacdac::instance() {
        return &instance_;
    }



    void Jacdac::onTick() {
        UNREACHABLE; // this should never be called, all the functionality is in the JacdacImpl class
    }



    void Jacdac::doEnable() {
        // claim sm and load programs, do not initialize the programs yet - we do that when we want to use them
        sm_ = pio_claim_unused_sm(JACDAC_PIO, true);
        txOffset_ = pio_add_program(JACDAC_PIO, &sws_tx_program);
        rxOffset_ = pio_add_program(JACDAC_PIO, &sws_rx_program);
        // get dma channel
        dma_ = dma_claim_unused_channel(true);
    }

    void Jacdac::doDisable() {

        pio_sm_set_enabled(JACDAC_PIO, sm_, false);
        pio_remove_program(JACDAC_PIO, &sws_tx_program, txOffset_);
        pio_remove_program(JACDAC_PIO, &sws_rx_program, rxOffset_);
        pio_sm_unclaim(JACDAC_PIO, sm_);
        dma_channel_unclaim(dma_);

    }


    bool Jacdac::doSend(uint8_t const * data, uint32_t numBytes) {
        // stop any ongoing transfers
        pio_sm_set_enabled(JACDAC_PIO, sm_, false);
        dma_channel_abort(dma_);
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
        dma_channel_config c = dma_channel_get_default_config(dma_);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
        channel_config_set_dreq(&c, pio_get_dreq(JACDAC_PIO, sm_, true));
        channel_config_set_read_increment(&c, true);
        channel_config_set_write_increment(&c, false);
        dma_channel_configure(dma_, &c, &JACDAC_PIO->txf[sm_], NULL, 0, false);
        dma_channel_transfer_from_buffer_now(dma_, data, numBytes);
        // start the PIO
        sws_tx_program_init(JACDAC_PIO, sm_, txOffset_, JACDAC_PIN);
        pio_sm_set_clock_speed(JACDAC_PIO, sm_, 8 * JACDAC_BAUDRATE);
        pio_sm_set_enabled(JACDAC_PIO, sm_, true);
        // wait for the DMA to finish and the PIO to be idle
        while (! pio_sm_is_stalled(JACDAC_PIO, sm_))
            ;
        pio_sm_set_enabled(JACDAC_PIO, sm_, false);
        // send the final BRK
        gpio::outputHigh(JACDAC_PIN);
        cpu::delayUs(1);
        gpio::low(JACDAC_PIN);
        cpu::delayUs(12);
        gpio::setAsInput(JACDAC_PIN);
        // TODO enable rx again

        return true;
    }

    void Jacdac::doReceive(uint8_t * data, uint32_t maxBytes, void (*callback)(uint8_t const * data, uint32_t numBytes)) {
        ASSERT(callback != nullptr);
        ASSERT(maxBytes >= sizeof(Frame));
        // stop any ongoing transfers
        pio_sm_set_enabled(JACDAC_PIO, sm_, false);
        dma_channel_abort(dma_);
        // we expect we are not sending anything at the moment, load the PIO sws receiver, configure the DMA and start the PIO. 
        gpio::setAsInput(JACDAC_PIN);
        // configure the DMA
        dma_channel_config c = dma_channel_get_default_config(dma_);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
        channel_config_set_dreq(&c, pio_get_dreq(JACDAC_PIO, sm_, false));
        channel_config_set_read_increment(&c, false);
        channel_config_set_write_increment(&c, true);
        dma_channel_configure(dma_, &c, NULL, &JACDAC_PIO->rxf[sm_], maxBytes, false);
        // load & configure the PIO program
        sws_rx_program_init(JACDAC_PIO, sm_, rxOffset_, JACDAC_PIN);
        pio_sm_set_clock_speed(JACDAC_PIO, sm_, 8 * JACDAC_BAUDRATE);
        // enable the interrupt from the pio, which is fired when the end BRK is detected
        irq_set_exclusive_handler(PIO1_IRQ_0, jacdacRxDone);
        irq_set_enabled(PIO1_IRQ_0, true);
        // enable the PIO & DMA
        pio_sm_set_enabled(JACDAC_PIO, sm_, true);
        dma_channel_transfer_to_buffer_now(dma_, data, maxBytes);
    }

} // namespace rckid