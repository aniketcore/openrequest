#include "ui_loop.h"
#include <thread>
#include <memory>
#include<iostream>
int main()
{
    std::thread UIthread(UI::RunUILoop);
    UIthread.join();
}