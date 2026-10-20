#pragma once

#if defined(_WIN32)
  #if defined(OPENREQUEST_UI_BUILD)
    #define OPENREQUEST_UI_API __declspec(dllexport)
  #else
    #define OPENREQUEST_UI_API __declspec(dllimport)
  #endif
#else
  #define OPENREQUEST_UI_API __attribute__((visibility("default")))
#endif

namespace UI {

OPENREQUEST_UI_API int RunUILoop();

} // namespace UI

extern "C" {
    OPENREQUEST_UI_API int OpenRequest_RunUI();
}
