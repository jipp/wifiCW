#include "SignalDecoder.hpp"

SignalDecoder::SignalDecoder(void)
{
}

void SignalDecoder::pressing()
{
  startTimerPressed = NOW_TIME;
  durationTimerNotPressed = startTimerPressed - startTimerNotPressed;

  if (durationTimerNotPressed < ditAvg * 2.4)
    recalculateDit(durationTimerNotPressed, ditAvg);
}

void SignalDecoder::releasing()
{
  uint64_t threshold;

  startTimerNotPressed = NOW_TIME;
  durationTimerPressed = startTimerNotPressed - startTimerPressed;
  threshold = sqrt(dahAvg * ditAvg);

  if (durationTimerPressed > (ditAvg * 0.5) && durationTimerPressed < (dahAvg * 2.5))
  {
    if (durationTimerPressed < threshold)
    {
      recalculateDit(durationTimerPressed, ditAvg);
      code += ".";
    }
    else
    {
      recalculateDah(durationTimerPressed, ditAvg, dahAvg);
      code += "_";
    }
    status = Status::symbolReceived;
  }
}

void SignalDecoder::released()
{
  if (status == Status::symbolReceived)
  {
    durationTimerNotPressed = NOW_TIME - startTimerNotPressed;
    lacktime = 2.2;

    if (durationTimerNotPressed > lacktime * ditAvg)
    {
      status = Status::characterReceived;
      wpm = (wpm + (int)(7200 / (dahAvg + 3 * ditAvg))) / 2;
    }
  }

  if (status == Status::characterReceived or status == Status::waitingWordReceived)
  {
    durationTimerNotPressed = NOW_TIME - startTimerNotPressed;

    if (wpm > 35)
      lacktime = 6;
    else if (wpm > 30)
      lacktime = 5.5;
    else
      lacktime = 5;

    if (durationTimerNotPressed > lacktime * ditAvg)
    {
      status = Status::wordReceived;
    }
  }
}

void SignalDecoder::contactUpdate()
{
  if (oldStatus == false)
  {
    released();
  }
}

void SignalDecoder::contactStatus(bool status)
{
  if (oldStatus == true)
  {
    if (status == false)
    {
      releasing();
      oldStatus = false;
    }
  }

  if (oldStatus == false)
  {
    if (status == true)
    {
      pressing();
      oldStatus = true;
    }
    if (status == false)
    {
      released();
    }
  }
}

void SignalDecoder::recalculateDit(uint64_t duration, uint64_t dit)
{
  dit = (4 * dit + duration) / 5;
}

void SignalDecoder::recalculateDah(uint64_t duration, uint64_t dit, uint64_t dah)
{
  if (duration > 2 * dah)
  {
    dah = (dah + 2 * duration) / 3;
    dit = dit / 2 + dahAvg / 6;
  }
  else
  {
    dah = (3 * dit + dah + duration) / 3;
  }
}
