#pragma once


// https://stackoverflow.com/questions/75246900/sending-i2c-command-from-c-application
class i2c {
public:

    static bool initializeMaster() {
        // TODO set speed here too
        // make sure that a write followed by a read to the same address will use repeated start as opposed to stop-start

        handle_ = open("/dev/i2c-1", O_RDWR);
        return handle_ >= 0;
        // old code with pigpio
        //i2cSwitchCombined(true);
    }

    // static void initializeSlave(uint8_t address_) {}

    static bool masterTransmit(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize) {
        i2c_msg msgs[2];
        memset(msgs, 0, sizeof(msgs));
        unsigned nmsgs = 0;
        if (wsize > 0) {
            msgs[nmsgs].addr = static_cast<uint16_t>(address);
            msgs[nmsgs].buf = const_cast<uint8_t*>(wb);
            msgs[nmsgs].len = static_cast<uint16_t>(wsize);
            ++nmsgs;
        }
        if (rsize > 0) {
            msgs[nmsgs].addr = static_cast<uint16_t>(address);
            msgs[nmsgs].buf = rb;
            msgs[nmsgs].len = static_cast<uint16_t>(rsize);
            msgs[nmsgs].flags = I2C_M_RD;
            ++nmsgs;
        }
        // fake zero writes for checking if the chip exists
        if (nmsgs == 0) {
            msgs[nmsgs].addr = static_cast<uint16_t>(address);
            msgs[nmsgs].len = 0;
            ++nmsgs;
        }
        i2c_rdwr_ioctl_data wrapper = {
            .msgs = msgs,
            .nmsgs = nmsgs};
        return ioctl(handle_, I2C_RDWR, &wrapper) >= 0;
        /* // old code with pigpio
        int h = i2cOpen(1, address, 0);
        if (h < 0)
            return false;
        if (wsize != 0)
            if (i2cWriteDevice(h, (char*)wb, wsize) != 0) {
                i2cClose(h);
                return false;
            }
        if (rsize != 0)
            if (i2cReadDevice(h, (char *)rb, rsize) != 0) {
                i2cClose(h);
                return false;
            }
        return i2cClose(h) == 0;
        */
    }

    static inline int handle_ = -1; 

}; // i2c
