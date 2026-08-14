#include "ui_loop.h"
#include <thread>

int main()
{
    std::thread UIthread(UI::RunUILoop);
    UIthread.join();
}