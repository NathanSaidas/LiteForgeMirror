// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_ABSTRACT_ENGINE_APPLICATION_BASE_H
#define LF_ABSTRACT_ENGINE_APPLICATION_BASE_H

#include "Core/Common/API.h"
#include "Core/Reflection/Object.h"
#include "Runtime/Reflection/ReflectionTypes.h"

namespace lf {

class LF_ABSTRACT_ENGINE_API ApplicationBase : public Object
{
    DECLARE_CLASS(ApplicationBase, Object);
public:
    // **********************************
    // Called when the application is first started up. (After the bootstrap layer is initialized)
    // 
    // **********************************
    virtual void OnStart() = 0;
    // **********************************
    // Called just as the application is shutting down. (Before the bootstrap layer is terminated)
    // **********************************
    virtual void OnExit() = 0;
};

} // namespace lf

#endif // LF_ABSTRACT_ENGINE_APPLICATION_BASE_H