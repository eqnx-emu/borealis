/*
    Copyright 2021 natinusala

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <borealis/core/time.hpp>

namespace brls
{

void Ticking::updateTickings()
{
    // Update time
    static Time previousTime = 0;

    Time currentTime = getCPUTimeUsec();
    Time elapsed     = previousTime == 0 ? 0 : currentTime - previousTime;

    previousTime = currentTime;

#ifdef __SWITCH__
    // Every loop iteration presents one image on a 60 Hz screen, so step the
    // animations by whole refresh periods. The measured time jitters around
    // the vsync (acquire returns early or late), and stepping by it moves the
    // animations unevenly between the frames that are actually shown.
    // The rounding error is carried over, so the average speed stays right.
    static Time pending = 0;
    static Time carry   = 0;
    const Time period   = 16667;
    pending += elapsed;
    Time frames = (pending + period / 2) / period;
    if (frames < 1 && elapsed > 0)
        frames = 1;
    pending -= frames * period;
    if (pending < -period)
        pending = -period;
    carry += frames * period;
    Time delta = carry / 1000;
    carry -= delta * 1000;
#else
    Time delta = elapsed / 1000;
#endif

    // Update every running ticking, kill them and execute cb if they are finished
    // We have to clone the running tickings list to avoid altering it while
    // in the for loop (so if another ticking is started in a callback or during onUpdate())
    std::vector<Ticking*> tickings(Ticking::runningTickings);

    for (Ticking* ticking : tickings)
    {
        bool run = ticking->onUpdate(delta);

        ticking->tickCallback();

        if (!run)
            ticking->stop(true); // will remove the ticking from Ticking::runningTickings
    }
}

void Ticking::start()
{
    if (this->running)
        return;

    Ticking::runningTickings.push_back(this);

    this->running = true;

    this->onStart();
}

void Ticking::stop()
{
    this->stop(false);
}

void Ticking::stop(bool finished)
{
    if (!this->running)
        return;

    for (size_t i = 0; i < Ticking::runningTickings.size(); i++)
    {
        if (Ticking::runningTickings[i] == this)
        {
            Ticking::runningTickings.erase(Ticking::runningTickings.begin() + i);
            break;
        }
    }

    this->running = false;

    this->onStop();

    this->endCallback(finished);
}

void Ticking::setEndCallback(TickingEndCallback endCallback)
{
    this->endCallback = endCallback;
}

void Ticking::setTickCallback(TickingTickCallback tickCallback)
{
    this->tickCallback = tickCallback;
}

bool Ticking::isRunning()
{
    return this->running;
}

Ticking::~Ticking()
{
    this->stop();
}

void FiniteTicking::rewind()
{
    this->onRewind();
}

void FiniteTicking::reset()
{
    this->stop();
    this->onReset();
}

} // namespace brls
