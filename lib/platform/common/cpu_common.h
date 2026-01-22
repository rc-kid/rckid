class NoISR {
public:
    NoISR() { cli(); }
    ~NoISR() { sei(); }
};

#define NO_ISR(...) do { cpu::NoISR noIsrGuard__; __VA_ARGS__; } while (false)