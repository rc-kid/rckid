#include <hardware/dma.h>

#include <rckid/buffer.h>
#include <rckid/capabilities/jacdac.h>

#include <sws_rx.pio.h>
#include <sws_tx.pio.h>


#define JACDAC_BAUDRATE 1000000

/** By default we use pio1, which is set to service the lower pins.
 */
#define JACDAC_PIO pio2

/** Pin on which the Jacdac data signal is connected. 
 */
#define JACDAC_PIN 19

/** When defined, this controls the trigger pin for the Jacdac. In rx mode the trigger pin is set to 1 when we are sampling the data line.
 */
#define JACDAC_TRIGGER_PIN 18

/** When defined, this controls the pin that indicates when the Jacdac receiver is active, i.e. when the PIO is processing a frame. 
 */
#define JACDAC_RX_ACTIVE_PIN 17

namespace rckid {

    struct JacdacImpl {
        Jacdac jacdac;
        int sm;
        unsigned txOffset;
        unsigned rxOffset;
        int dma; 
        uint64_t eventTime = 0;
        volatile bool rxReady = false;
        // double buffer for receiving messages, large enough to store two frames
        DoubleBuffer<uint8_t> rxBuffer{sizeof(Jacdac::Frame)};
        //
        bool irqEnabledBefore = false;

        JacdacImpl();

        ~JacdacImpl();

    }; // JacdacImpl

    namespace {
        JacdacImpl * instance_;

        void jacdacRxDone();

        /** Resets the Jacdac instance. Aborts any ongoing transfers (rx or tx)
         */
        void jacdacReset() {
#ifdef JACDAC_RX_ACTIVE_PIN
            gpio::outputLow(JACDAC_RX_ACTIVE_PIN);
#endif
            // stop any ongoing transfers & reset the state
            gpio_set_irq_enabled(JACDAC_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);
            pio_sm_set_enabled(JACDAC_PIO, instance_->sm, false);
            dma_channel_abort(instance_->dma);
        }

        /** Interrupt on jacdac data pin, which we use to detect break condition.
         
            Falling edge can be the beginning of the break condition so we just detect the time of the event. 

            Rising edge after the required BREAK condition duration (12us) is the end of the break condition so we initialize the PIO and start the DMA. 
         */
        void __not_in_flash_func(jacdacRxIrq)() {
            uint32_t eventMask = gpio_get_irq_event_mask(JACDAC_PIN);
            gpio_acknowledge_irq(JACDAC_PIN, eventMask);
            uint64_t eventTime = time::uptimeUs();
            Jacdac::rxStatus = 2;
            if (eventMask & GPIO_IRQ_EDGE_FALL) {
                instance_->eventTime = eventTime;
            } 
            if (eventMask & GPIO_IRQ_EDGE_RISE) {
                if (instance_->eventTime + 10 <= eventTime) {
                    // disable the pin irq handler (all handled by the PIO now)
                    gpio_set_irq_enabled(JACDAC_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);

                    // load and configure the PIO program for SWS RX
#ifdef JACDAC_TRIGGER_PIN                    
                    sws_rx_program_init(JACDAC_PIO, instance_->sm, instance_->rxOffset, JACDAC_PIN, JACDAC_TRIGGER_PIN);
#else
                    sws_rx_program_init(JACDAC_PIO, instance_->sm, instance_->rxOffset, JACDAC_PIN);
#endif
                    pio_sm_set_clock_speed(JACDAC_PIO, instance_->sm, 8 * JACDAC_BAUDRATE);
                    // enable the interrupt from the pio, which is fired when the end BRK is detected
                    irq_set_exclusive_handler(PIO2_IRQ_0, jacdacRxDone);
                    irq_set_enabled(PIO2_IRQ_0, true);
                    pio_set_irq0_source_enabled(JACDAC_PIO, pis_interrupt0, true);

                    // enable the PIO & DMA
                    pio_sm_set_enabled(JACDAC_PIO, instance_->sm, true);
                    dma_channel_transfer_to_buffer_now(instance_->dma, instance_->rxBuffer.back().data(), sizeof(Jacdac::Frame));

                    #ifdef JACDAC_RX_ACTIVE_PIN
                    gpio::outputHigh(JACDAC_RX_ACTIVE_PIN);
                    #endif

                    Jacdac::rxStatus = 3;
                }
            }
        }

        void __not_in_flash_func(jacdacRxStart)() {
            // stop any ongoing transfers & reset the state
            jacdacReset();
            // initialize the DMA so as not to waste time in the IRQ
            dma_channel_config c = dma_channel_get_default_config(instance_->dma);
            channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
            channel_config_set_dreq(&c, pio_get_dreq(JACDAC_PIO, instance_->sm, false));
            channel_config_set_read_increment(&c, false);
            channel_config_set_write_increment(&c, true);
            // write address is nullptr (will be set by transfer to buffer below), read address is the RX FIFO of the pio
            dma_channel_configure(instance_->dma, &c, nullptr, &JACDAC_PIO->rxf[instance_->sm], sizeof(Jacdac::Frame), false);
            // set the pin as input and wait for break condition (low for at least 12us)
            // TODO pull-up if master?
            gpio::setAsInput(JACDAC_PIN); 
            // enable interrupts on edges so that we can detect the break condition
            gpio_set_irq_enabled(JACDAC_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
            instance_->eventTime = time::uptimeUs();
           // Jacdac::rxStatus = 1;
        }

        void __not_in_flash_func(jacdacRxDone)() {
            Jacdac::rxStatus = 4;
            // wait for the pio rx queue to be empty, which means the pio has finished processing the frame (and the DMA)
            while (! pio_sm_is_rx_fifo_empty(JACDAC_PIO, instance_->sm))
                ;
            Jacdac::rxStatus = 5;
            Jacdac::receivedPackets++;
            // by the time we get here we can assume that the DMA has transferred everything we need, no need to wait for anything. Store the size of the received frame in the back buffer (transfer count decrements from the initial value, which we know was set to frame size)
            instance_->rxBuffer.back().setUsed(sizeof(Jacdac::Frame) - dma_hw->ch[instance_->dma].transfer_count);
            uint32_t frameSize = instance_->rxBuffer.back().used();
            Jacdac::receivedBytes += frameSize;
            // swap front & back buffers and set the rxReady flag (only if the front buffer is already processed)
            if (instance_->rxReady == false || true) { // TODO DELETE
                // swap the buffers
                instance_->rxBuffer.swap();
                // verify the frame is valid (size-wise, which is really fast and can be done inside ISR)
                Jacdac::Frame * f = reinterpret_cast<Jacdac::Frame*>(instance_->rxBuffer.front().data());
                if ((f->size + 12) != frameSize) {
                    Jacdac::errors++;
                    LOG(LL_ERROR, "Jacdac: Frame size mismatch, expected " << (f->size + 12) << ", got " << frameSize);
                } else {
                    LOG(LL_INFO, "Jacdac: Frame received, size " << frameSize << ", device id " << f->device_identifier << ", flags " << (int)f->flags);
                    instance_->rxReady = true;
                }
            } else {
                // have to drop the back buffer
            }
            // clear the interrupt
            pio_interrupt_clear(JACDAC_PIO, 0);
            // after the frame was processd, time to restart the received
            jacdacRxStart();

            // TODO process the frame here perhaps? 
            Jacdac::rxStatus = 6;
        }



    }; // anonynous namespace

    JacdacImpl::JacdacImpl() {
        gpio::setAsInput(JACDAC_PIN); // TODO pullup if master>?
        gpio_add_raw_irq_handler(JACDAC_PIN, &jacdacRxIrq);
        irqEnabledBefore = irq_is_enabled(IO_IRQ_BANK0);
        irq_set_enabled(IO_IRQ_BANK0, true);
        // claim sm and load programs, do not initialize the programs yet - we do that when we want to use them
        sm = pio_claim_unused_sm(JACDAC_PIO, true);
        txOffset = pio_add_program(JACDAC_PIO, &sws_tx_program);
        rxOffset = pio_add_program(JACDAC_PIO, &sws_rx_program);
        // get dma channel
        dma = dma_claim_unused_channel(true);

#ifdef JACDAC_RX_ACTIVE_PIN
        gpio::setAsOutput(JACDAC_RX_ACTIVE_PIN);
        gpio::outputLow(JACDAC_RX_ACTIVE_PIN);
#endif
    }

    JacdacImpl::~JacdacImpl() {
        gpio_set_irq_enabled(JACDAC_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);
        gpio_remove_raw_irq_handler(JACDAC_PIN, &jacdacRxIrq);
        if (! irqEnabledBefore)
            irq_set_enabled(IO_IRQ_BANK0, false);
        pio_sm_set_enabled(JACDAC_PIO, instance_->sm, false);
        pio_remove_program(JACDAC_PIO, &sws_tx_program, instance_->txOffset);
        pio_remove_program(JACDAC_PIO, &sws_rx_program, instance_->rxOffset);
        pio_sm_unclaim(JACDAC_PIO, instance_->sm);
        dma_channel_unclaim(instance_->dma);
#ifdef JACDAC_RX_ACTIVE_PIN
        gpio::setAsInput(JACDAC_RX_ACTIVE_PIN);
#endif
    }



    Jacdac * Jacdac::instance() {
        if (instance_ == nullptr)
            instance_ = new JacdacImpl();
        return & (instance_->jacdac);
    }


    void Jacdac::sendFrame(uint8_t const * data, uint32_t numBytes) {
        doSend(data, numBytes);
    }

    void Jacdac::onTick() {
        // TODO
    }

    void Jacdac::doEnable() {
        // start the receiver
        doReceive();
    }

    void Jacdac::doDisable() {

    }

    bool Jacdac::doSend(uint8_t const * data, uint32_t numBytes) {
        // stop any ongoing transfers
        jacdacReset();
        // pre-initializing the PIO
        sws_tx_program_init(JACDAC_PIO, instance_->sm, instance_->txOffset, JACDAC_PIN);
        pio_sm_set_clock_speed(JACDAC_PIO, instance_->sm, 8 * JACDAC_BAUDRATE);
        // pre-initialize the DMA
        dma_channel_config c = dma_channel_get_default_config(instance_->dma);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
        channel_config_set_dreq(&c, pio_get_dreq(JACDAC_PIO, instance_->sm, true));
        channel_config_set_read_increment(&c, true);
        channel_config_set_write_increment(&c, false);
        // write addressis the TX FIFO, read address is null here, set by transfer from buffer below explicitly
        dma_channel_configure(instance_->dma, &c, &JACDAC_PIO->txf[instance_->sm], nullptr, 0, false);
        // start the DMA (this should fill the FIFO so that the PIO can start immediately)
        dma_channel_transfer_from_buffer_now(instance_->dma, data, numBytes);

        // wait for the Jacdac data line to be idle
        // TODO this is wrong, the bus is idle when high consecutively for larger period of time
        gpio::setAsInput(JACDAC_PIN);
        while (gpio::read(JACDAC_PIN) == false)
            ;
        cpu::DisableInterruptsGuard _;
        // send the packet start command
        gpio::outputLow(JACDAC_PIN);
        cpu::delayUs(12, /* precise */ true);
        // wait for 49us so that we can start sending the data TODO why 49us?
        gpio::setAsInput(JACDAC_PIN);
        cpu::delayUs(49, /* precise */ true);
        // and start the transfer
        pio_gpio_init(JACDAC_PIO, JACDAC_PIN);
        pio_sm_set_enabled(JACDAC_PIO, instance_->sm, true);
        // wait for the transfer to be done and the PIO to be idle
        while (dma_channel_is_busy(instance_->dma) || !pio_sm_is_stalled(JACDAC_PIO, instance_->sm))
            ;
        pio_sm_set_enabled(JACDAC_PIO, instance_->sm, false);
        // send the final BRK
        gpio::outputHigh(JACDAC_PIN);
        cpu::delayUs(1, /* precise */ true);
        gpio::low(JACDAC_PIN);
        cpu::delayUs(12, /* precise */ true);
        gpio::setAsInput(JACDAC_PIN);

        return true;

    }

    void Jacdac::doReceive() {
        jacdacRxStart();
    }

} // namespace rckid