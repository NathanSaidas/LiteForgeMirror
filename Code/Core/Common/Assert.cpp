#include "Assert.h"

namespace lf {

static void NullHandler(const char*, ErrorCode, ErrorApi) {}

AssertCallback gAssertCallback = NullHandler;
CrashCallback  gCrashCallback = NullHandler;
BugCallback    gReportBugCallback = NullHandler;

}