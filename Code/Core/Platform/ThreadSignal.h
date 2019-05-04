// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_THREAD_SIGNAL_H
#define LF_CORE_THREAD_SIGNAL_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

namespace lf {

class LF_CORE_API ThreadSignal
{
public:

    ThreadSignal() : 
    mValue(0),
    mValueDummy(0)
    {}

    void Wait();
    void WakeOne();
    void WakeAll();

private:
    volatile Int32 mValue;
    Int32 mValueDummy;
};

}



#endif // LF_CORE_THREAD_SIGNAL_H