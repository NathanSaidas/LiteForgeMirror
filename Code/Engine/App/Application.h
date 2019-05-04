// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_ENGINE_APPLICATION_H
#define LF_ENGINE_APPLICATION_H

#include "Core/Common/API.h"
#include "AbstractEngine/App/ApplicationBase.h"

namespace lf {

class LF_ENGINE_API Application : public ApplicationBase
{
    DECLARE_CLASS(Application, ApplicationBase);
public:
    // **********************************
    // Called when the application is first started up. (After the bootstrap layer is initialized)
    // 
    // **********************************
    virtual void OnStart();
    // **********************************
    // Called just as the application is shutting down. (Before the bootstrap layer is terminated)
    // **********************************
    virtual void OnExit();
};

} // namespace lf

#endif // LF_ENGINE_APPLICATION_H