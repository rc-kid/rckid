#pragma once

#include <time_point.h>

#include <rckid/rckid.h>

#include "config.h"

namespace rckid {

    /** I2C Communication 
     
        RCKid supports asynchronnous I2C communication (especially with the AVR) where commands can be enqueued and will be sent by the I2C HW buffers with minimal CPU intervention. Alternatively, the I2C transactions can be blocking - since we have no multithreading up to 1 blocking I2C communication per core can exist.
        
        The I2C comms maintain a ring buffer for async transactions, which essentially allows the app to use fire & forget for AVR commands that control the device behavior. If the buffer becomes full, the async request will busy wait until a slot becomes free.

        Async transactions also support automatic retries on NACKs and other I2C abortions with linear backoff and bounded number of retries. 

     */
    class i2c {
    public:
        /** Magic value send to async callbacks in case transaction was aborted.
         */
        static constexpr int32_t TransactionError = -1;

        /** Callback function for I2C transactions. Called when the transaction is processed. Takes the number of bytes returned from the slave as its argument. At the call, the bytes are still in I2C HW buffers and have to be read using the getTransactionResponse() function below. 
         */
        using Callback = std::function<void(int32_t)>;

        /** Initialzies the I2C communucations. 
         
            Enables the I2C HW and sets the interrupt. 
         */
        static void initialize() {
            // initialize the I2C bus we use to talk to AVR & peripherals (unlike mkII this is not expected to be user accessible)
            i2c_init(i2c0, RCKID_I2C_SPEED); 
            i2c0->hw->intr_mask = 0; // disable interrupts for now
            gpio_set_function(RP_PIN_I2C_SDA, GPIO_FUNC_I2C);
            gpio_set_function(RP_PIN_I2C_SCL, GPIO_FUNC_I2C);
            irq_set_exclusive_handler(I2C0_IRQ, irqHandler);
            irq_set_enabled(I2C0_IRQ, true);
            // make the I2C IRQ priority larger than that of the DMA (0x80) to ensure that I2C comms do not have to wait for render done if preparing data takes longer than sending them
            irq_set_priority(I2C0_IRQ, 0x40); 
        }

        static bool queueEmpty() { return ri_ == wi_; }

        static bool full() { return (wi_ + 1) % RCKID_I2C_ASYNC_SLOTS == ri_; }

        /** Reads the given number of response bytes from the I2C HW buffers and stores them in the given buffer. The buffer must be at least numBytes long.
         */
        static void getTransactionResponse(uint8_t * into, uint8_t numBytes) {
            while (numBytes-- > 0)
                *(into++) = i2c0->hw->data_cmd;
        }

        /** Shorthand for sending AVR commands. Takes the command and sends it to the AVR chip as byte array. 
         */
        template<typename T>
        static void sendAvrCommand(T const & cmd) {
            LOG(LL_INFO, "avr cmd: " << cmd.id);
            transmitAsync(RCKID_AVR_I2C_ADDRESS, reinterpret_cast<uint8_t const *>(&cmd), sizeof(T));
        }

        /** Asynchronously tramsits the specified I2C transaction.

            Enqueues the transaction into the I2C ring buffer. When the transaction will be processed, calls the callback function, which can read any received data. Due to RP2350 I2C HW limitations, an async transaction can move at most 16 bytes (write + read).
         */
        static void transmitAsync(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t rsize = 0, Callback cb = nullptr) {
            uint32_t next = (wi_ + 1) % RCKID_I2C_ASYNC_SLOTS;
            // if full, wait
            while (next == ri_) // safe volatile uint32_t is atomic on RP2350
                yield();
            // enqueue
            new (ring_ + wi_) Transaction{address, wb, wsize, rsize, cb};
            bool trigger = false;
            {
                cpu::DisableInterruptsGuard g{};
                if (ri_ == wi_)
                    trigger = true;
                wi_ = next;
            }
            // if we have enqueued into empty (that means I2C is now idle) we need to trigger the transaction manually, otherwise it will get triggered when ready
            if (trigger)
                triggerTransaction();
        }

        /** Synchronously transmits the given I2C transaction.
         
            Waits for all currently enqueued async transactions to finish, then processes the given transaction synchronously. This takes time, but as it has direct access to the I2C hardware can copy larger quantities of data. 
         */
        static void transmitSync(uint8_t address, uint8_t const * wb, uint32_t wsize, uint8_t * rb = nullptr, uint32_t rsize = 0) {
            uint32_t next = (wi_ + 1) % RCKID_I2C_ASYNC_SLOTS;
            // if full, wait
            while (next == ri_) // safe volatile uint32_t is atomic on RP2350
                yield();
            // enqueue
            new (ring_ + wi_) Transaction{nullptr};
            // update the read index
            uint32_t rx = wi_;
            {
                cpu::DisableInterruptsGuard g{};
                wi_ = next;
            }
            // wait for the read index to get to the enqueued transaction
            while (rx != ri_)
                yield();
            // we are now in control of the I2C hardware, disable interrupts
            i2c0->hw->clr_intr;
            i2c0->hw->intr_mask = 0;
            // and process the blocking operation
            if (wsize != 0)
                i2c_write_blocking(i2c0, address, wb, wsize, rsize != 0);
            if (rsize != 0)
                i2c_read_blocking(i2c0, address, rb, rsize, false);
            // move to next ri and trigger if not empty
            moveToNext();
        }

        /** Enqueues given callback function to be called when I2C hardware is idle so that the callback itself can take full control of the hardware.
         */
        static void enqueue(Callback cb) {
            ASSERT(cb != nullptr); // this would deadlock
            uint32_t next = (wi_ + 1) % RCKID_I2C_ASYNC_SLOTS;
            // if full, wait
            while (next == ri_) // safe volatile uint32_t is atomic on RP2350
                yield();
            // enqueue
            new (ring_ + wi_) Transaction{cb};
            // update the read index
            {
                cpu::DisableInterruptsGuard g{};
                wi_ = next;
            }
            // running yield triggers the sync transaction immediately if the queue was empty
            onYield();
        }

        /** On yield handler for non-isr periodic checking.

            The onYield handler helps the I2C driver with two tasks: 
            
            1) it re-triggers backedoff previously aborted async transactions if the time is right (we can't do this from the ISR as there are long backoff waits involved). The retrigger mechanism in the ISR also ansures that retrigger time is never 0 if required

            2) for sync transactions, we take use this to call their callback (i.e. sync transaction is never triggered from the ISR) so that the sync waits will happen in main loop. This requires disabling the interrupts ensuring the I2C is fully callback controlled and then calling the callback.
         */
        static void onYield() {
            {
                cpu::DisableInterruptsGuard g{};
                if ((retrigger_ != 0) && (TimePoint::now() >= retrigger_)) {
                    triggerTransaction();
                    return;
                }
                if (ri_ == wi_)
                    return;
                if (ring_[ri_].address != MAGIC_SYNC_TRANSACTION)
                    return;
                if (ring_[ri_].callback == nullptr)
                    return;
            }
            // take control of the I2C hardware, disable interrupts
            i2c0->hw->clr_intr;
            i2c0->hw->intr_mask = 0;
            // call the callback - we know it exists, we know ri has not moved (because we are stuck at sync block that does not get triggered)
            ring_[ri_].callback(0);
            // when callback done, move to next, if any
            moveToNext();
        }

    private:

        static constexpr uint8_t MAGIC_SYNC_TRANSACTION = 0xff;

        PACKED(struct Transaction {
            uint8_t address;
            uint8_t writeBuffer[16];
            uint8_t writeSize;
            uint8_t readSize;
            Callback callback;

            Transaction() = default;

            /** Creates async transaction.
             */
            Transaction(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t rsize = 0, Callback cb = nullptr):
                address{address},
                writeSize{wsize},
                readSize{rsize},
                callback{cb} 
            {
                ASSERT(rsize + wsize <= 16);
                if (wsize != 0)
                    memcpy(writeBuffer, wb, wsize);
            }

            Transaction(Callback cb):
                address{MAGIC_SYNC_TRANSACTION},
                callback{cb} {
            }


        }); // i2c::Transaction

        static constexpr uint32_t I2C_TX_ABORT = 0x40;
        static constexpr uint32_t I2C_RD_REQ = 0x20;
        static constexpr uint32_t I2C_TX_EMPTY = 0x10;
        static constexpr uint32_t I2C_TX_OVER = 0x08;
        static constexpr uint32_t I2C_RX_FULL = 0x04;

        /** I2C IRQ handler. 
         
            Called when the transaction is done, executes the callback function of the transaction and moves to the next transaction in the queue. 

            TX_ABRT = transmit was aborted (bit 6)

            RX_FULL = what was to be received was received (bit 2) 0x04

         */
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
                // deal with teh callback
                if (t.callback)
                    t.callback(t.readSize); // TODO should this be the actual number of bytes received in the buffer
                // move to the next transaction
                moveToNext();
            }
        }

        /** Moves to next transaction, triggering it if the queue is not empty.
         */
        static void moveToNext() {
            // reset the attempts counter
            attempts_ = 0;
            ring_[ri_].callback = nullptr;
            // move to the next transaction
            bool trigger = false;
            {
                // we are already in an interrupt, no need to clear them
                ri_ = (ri_ + 1) % RCKID_I2C_ASYNC_SLOTS;
                if (ri_ != wi_)
                    trigger = true;
            }
            if (trigger)
                triggerTransaction();
        }

        /** Begins the async transmit of the transaction. 
         
            Performs I2C reset and then fills the I2C HW buffer with the send/receive commands and sets the I2C interrupt conditions for successful transmission end (either enough bytes sent, or enough bytes received). 
         */
        static void triggerAsync(Transaction & t) {
            i2c0->hw->enable = 0;
            i2c0->hw->tar = t.address;
            // TODO is this correct - shouldn't we enable only after all the data is written and interrupts are enabled?
            i2c0->hw->enable = 1;
            // if there are data to send, write them first. Do not forget to set the stop bit if no data to red
            if (t.writeSize > 0) {
                for (uint32_t i = 0, e = t.writeSize - 1; i <= e; ++i)
                    i2c0->hw->data_cmd = t.writeBuffer[i] | ((i == e && t.readSize == 0) ? I2C_IC_DATA_CMD_STOP_BITS : 0);
            }
            // if there are data to read, write the recv bits
            if (t.readSize > 0) {
                for (uint32_t i = 0, e = t.readSize - 1; i <= e; ++i) {
                    uint32_t cmd = I2C_IC_DATA_CMD_CMD_BITS;
                    if (i == 0 && t.writeSize != 0)
                        cmd |= I2C_IC_DATA_CMD_RESTART_BITS;
                    if (i == e)
                        cmd |= I2C_IC_DATA_CMD_STOP_BITS;
                    i2c0->hw->data_cmd = cmd;
                }
            }
            // depending on the packet, set the conditions for interrupt when done, or aborted. If we are not reading, we set the TX threshold to 0 (i.e. all tx bytes sent) and enable the interrupt for TX empty and TX abort. When we are reading, then set the RX threshold to number of bytes to read minus one (its when the RX buffer is *above* the threshold) and enable RX full and TX abort. 
            // TODO could this happen *before* we get here? maybe disable interrupts for the function first?
            // TODO ensure that the TX abort is also set when RX won't read everything we wanted.
            if (t.readSize == 0) {
                i2c0->hw->rx_tl = 0;
                i2c0->hw->intr_mask = I2C_IC_INTR_MASK_M_TX_EMPTY_BITS | I2C_IC_INTR_MASK_M_TX_ABRT_BITS;
            } else {
                i2c0->hw->rx_tl = t.readSize - 1;
                i2c0->hw->intr_mask = I2C_IC_INTR_MASK_M_RX_FULL_BITS | I2C_IC_INTR_MASK_M_TX_ABRT_BITS;
            }
        }

        static void triggerTransaction() {
            // clear the re-trigger mark
            retrigger_ = 0;
            Transaction & t = ring_[ri_];
            if (t.address != MAGIC_SYNC_TRANSACTION)
                triggerAsync(t);
        }

        // transaction ring buffer
        static inline Transaction ring_[RCKID_I2C_ASYNC_SLOTS];
        // index at which new transaction will be enqeueued 
        static inline volatile uint32_t wi_ = 0;
        // index at which the transactions are transmitted/processed
        static inline volatile uint32_t ri_ = 0;
        // attempts made with current packet (reset by moveToNext)
        static inline volatile uint32_t attempts_ = 0;
        // retrigger time in us (32bit), or 0 if no retrigger necessary
        static inline TimePoint retrigger_;

    }; // class i2c

} // namespace rckid

