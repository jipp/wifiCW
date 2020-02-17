#ifndef SIGNAL_DECODER_HPP_
#define SIGNAL_DECODER_HPP_

// decoder code taken from: https://github.com/oe1wkl/Morserino-32 /blob/master/Software/src/morse_3_v2.3/morse_3_v2.3.ino

#include <chrono>
#include <cmath>
#include <string>

#define NOW_TIME std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()

class SignalDecoder
{
public:
    uint8_t wpm = 15;
    std::string code;
    enum Status
    {
        waiting,
        symbolReceived,
        characterReceived,
        waitingWordReceived,
        wordReceived
    };
    Status status = Status::waiting;
    SignalDecoder(void);
    void pressing();
    void releasing();
    void released();
    void contactUpdate();
    void contactStatus(bool status);

private:
    uint64_t startTimerPressed = 0;
    uint64_t durationTimerPressed = 0;
    uint64_t startTimerNotPressed = 0;
    uint64_t durationTimerNotPressed = 0;
    const uint64_t dit = 60;
    const uint64_t dah = 3 * dit;
    uint64_t ditAvg = dit;
    uint64_t dahAvg = dah;
    float lacktime = 2.2;
    void recalculateDit(uint64_t duration, uint64_t dit);
    void recalculateDah(uint64_t duration, uint64_t dit, uint64_t dah);
    bool oldStatus = false;
};

#endif