static size_t transfer(uint8_t const * tx, uint8_t * rx, size_t numBytes) {
    for (size_t i = 0; i < numBytes; ++i)
        *(rx++) = transfer(*(tx++));
    return numBytes;
}

static void send(uint8_t const * data, size_t numBytes) {
    for (size_t i = 0; i < numBytes; ++i)
        transfer(*(data++));
}

static void receive(uint8_t * data, size_t numBytes) {
    for (size_t i = 0; i < numBytes; ++i)
        *(data++) = transfer(0);
}
