#pragma once

#include <rckid/rckid.h>
#include "backend_internals.h"

namespace rckid {

    /** I2C Communication 
     
        RCKid supports asynchronnous I2C communication (especially with the AVR) where commands can be enqueued and will be sent by the I2C HW buffers with minimal CPU intervention. Alternatively, the I2C transactions can be blocking - since we have no multithreading up to 1 blocking I2C communication per core can exist. 
     */
    class i2c {
    public:
        /** Callback function for I2C transactions. Called when the transaction is processed. Takes the number of bytes returned from the slave as its argument. At the call, the bytes are still in I2C HW buffers and have to be read using the getTransactionResponse() function below. 
         */
        typedef void (*Callback)(uint8_t);

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

        /** Enqueues I2C transaction that can write and read from given address. When the transaction finishes, calls the optional callback function that may read the bytes received (if any) using the readResponse function.
         */
        static void enqueue(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t rsize = 0, Callback cb = nullptr) {
            ASSERT(wsize <= 16 && rsize <= 16);
            Transaction * t = reinterpret_cast<Transaction*>(malloc(sizeof(Transaction) + wsize));
            t->address = address;
            t->wsize = wsize;
            t->rsize = rsize;
            t->cb = cb;
            t->next = nullptr;
            // copy the wdata
            if (wsize > 0) {
                uint8_t * twb = reinterpret_cast<uint8_t *>(t + 1);
                memcpy(twb, wb, wsize);
            }
            // enqueue the transaction in the list
            t->enqueue();
            // if this was the first transaction in the queue, proceed with immediate transmit
            if (current_ == t) 
                t->transmit();

        }

        /** Blocking I2C transaction. Enqueues the packet, but waits for its completion, returning the read parts into the provided buffer. 
         */
        static void enqueueAndWait(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb = nullptr, uint8_t rsize = 0) {
            ASSERT(wsize <= 16 && rsize <= 16);
            volatile bool done = false;
            BlockingTransaction * t = new BlockingTransaction{};
            t->address = address;
            t->wsize = wsize;
            t->rsize = rsize;
            t->cb = enqueueAndWaitCallback;
            t->next = nullptr;
            t->wb = wb;
            t->rb = rb;
            t->done = &done;
            // enqueue the transaction in the list
            t->enqueue();
            // if this was the first transaction in the queue, proceed with immediate transmit
            if (current_ == t)
                t->transmit();
            // wait for the transaction to finish
            while (!done)
                yield();
        }

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
            enqueue(RCKID_AVR_I2C_ADDRESS, reinterpret_cast<uint8_t const *>(&cmd), sizeof(T));
        }

    private:

        /** I2C Transaction. 
         
            The transaction consists of a number of bytes to send and number of bytes to receive. Due to the limitations of the I2C HW at the RPI chip, a single transaction can send up to 16 bytes and receive up to 16 bytes as well giving total transaction max size of 32 bytes. 

         */
        PACKED(class Transaction {
        public:
            uint8_t address;
            uint8_t wsize;
            uint8_t rsize;
            Callback cb;
            Transaction * next;

            /** Returns the data to be written to the slave as part of the transaction. Those are stored directly after the transaction object. */
            // the warning is false positive (the cb check above ensures that we are in fact a blocking transaction and hence not outside bounds)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
            uint8_t const * wdata() const {
                if (cb == enqueueAndWaitCallback)
                    return reinterpret_cast<BlockingTransaction const *>(this)->wb;
                else 
                    return reinterpret_cast<uint8_t const *>(this + 1);
            }
#pragma GCC diagnostic pop

            /** Ennqueues the transaction after the last transaction in the queue.
             */
            void enqueue() {
                cpu::DisableInterruptsGuard g;
                if (current_ == nullptr) {
                    ASSERT(last_ == nullptr);
                    current_ = this;
                    last_ = this;
                } else {
                    last_->next = this;
                    last_ = this;
                }
            }

            /** Begins the async transmit of the transaction. 
             
                Performs I2C reset and then fills the I2C HW buffer with the send/receive commands and sets the I2C interrupt conditions for successful transmission end (either enough bytes sent, or enough bytes received). 
             */
            void transmit() {
                i2c0->hw->enable = 0;
                i2c0->hw->tar = address;
                // TODO is this correct - shouldn't we enable only after all the data is written and interrupts are enabled?
                i2c0->hw->enable = 1;
                // if there are data to send, write them first. Do not forget to set the stop bit if no data to red
                if (wsize > 0) {
                    uint8_t const * x = wdata();
                    for (uint32_t i = 0, e = wsize - 1; i <= e; ++i)
                        i2c0->hw->data_cmd = x[i] | ((i == e && rsize == 0) ? I2C_IC_DATA_CMD_STOP_BITS : 0);
                }
                // if there are data to read, write the recv bits
                if (rsize > 0) {
                    for (uint32_t i = 0, e = rsize - 1; i <= e; ++i) {
                        uint32_t cmd = I2C_IC_DATA_CMD_CMD_BITS;
                        if (i == 0 && wsize != 0)
                            cmd |= I2C_IC_DATA_CMD_RESTART_BITS;
                        if (i == e)
                            cmd |= I2C_IC_DATA_CMD_STOP_BITS;
                        i2c0->hw->data_cmd = cmd;
                    }
                }
                // depending on the packet, set the conditions for interrupt when done, or aborted. If we are not reading, we set the TX threshold to 0 (i.e. all tx bytes sent) and enable the interrupt for TX empty and TX abort. When we are reading, then set the RX threshold to number of bytes to read minus one (its when the RX buffer is *above* the threshold) and enable RX full and TX abort. 
                // TODO could this happen *before* we get here? maybe disable interrupts for the function first?
                // TODO ensure that the TX abort is also set when RX won't read everything we wanted.
                if (rsize == 0) {
                    i2c0->hw->rx_tl = 0;
                    i2c0->hw->intr_mask = I2C_IC_INTR_MASK_M_TX_EMPTY_BITS | I2C_IC_INTR_MASK_M_TX_ABRT_BITS;
                } else {
                    i2c0->hw->rx_tl = rsize - 1;
                    i2c0->hw->intr_mask = I2C_IC_INTR_MASK_M_RX_FULL_BITS | I2C_IC_INTR_MASK_M_TX_ABRT_BITS;
                }
            }
        }); 

        /** Blocking transaction. 
         
            Like transaction, but instead of write data being stored directly after the transaction and read data kept in the HW buffers, the blocking transaction stores write and read data pointers as well as a pointer to a volatile done flag (it being a boolean is also atomic on the architecture). 
         */
        PACKED(class BlockingTransaction : public Transaction {
        public:
            uint8_t const * wb;
            uint8_t * rb;
            volatile bool * done; 
        });

        /** Callback for the enqueue and wait transaction. 
         */
        static void enqueueAndWaitCallback(uint8_t numBytes) {
            BlockingTransaction * t = reinterpret_cast<BlockingTransaction*>(current_);
            getTransactionResponse(t->rb, numBytes);
            *(t->done) = true;
        }

        /** I2C IRQ handler. 
         
            Called when the transaction is done, executes the callback function of the transaction and moves to the next transaction in the queue. 
         */
        static void __not_in_flash_func(irqHandler)() {
            uint32_t cause = i2c0->hw->intr_stat;
            i2c0->hw->clr_intr;
            // disable the I2C interrupts (otherwise they might fire again immediately despite clearing, especially the TX_EMPTY one)
            i2c0->hw->intr_mask = 0;
            LOG(LL_I2C, "IRQ " << hex(cause) << " -- " << hex(i2c0->hw->intr_stat));
            Transaction * t = current_;
            // deal with the callback
            if (t->cb != nullptr) {
                // TODO can we get the real number of bytes read somehow if there was an error and the number is smaller?
                t->cb(current_->rsize);
            }
            // move to the next transaction
            current_ = current_->next;
            if (last_ == t)  {
                ASSERT(current_ == nullptr);
                last_ = nullptr;
            }
            if (current_ != nullptr) {
                ASSERT(last_ != nullptr);
                current_->transmit();
            }
            // and finally delete the already processed transaction
            delete t;
        }

        // current transaction (the one being processed)
        static inline Transaction * current_ = nullptr;
        // last transaction (after which new transactions are appended)
        static inline Transaction * last_ = nullptr;

    }; // rckid::i2c

} // namespace rckid

