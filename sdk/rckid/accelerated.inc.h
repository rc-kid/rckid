#define MEM_FILL_8 void memFill(uint8_t * buffer, uint32_t size, uint8_t value) { \
    while (size-- != 0) \
        *(buffer++) = value; \
}

#define MEM_FILL_16 void memFill(uint16_t * buffer, uint32_t size, uint16_t value) { \
    while (size-- != 0) \
        *(buffer++) = value; \
}

#define MEM_FILL_32 void memFill(uint32_t * buffer, uint32_t size, uint32_t value) { \
    while (size-- != 0) \
        *(buffer++) = value; \
}
