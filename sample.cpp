//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "ChakraCore.h"
#include "JSHost.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <queue>
#include <time.h>
#include <thread>
#include <iostream>
#include <chrono>
#include <mutex>
#include <condition_variable>

using namespace std;

std::atomic_int Task::ID{0};

void createJSThread()
{
    JSHost host;
    if (!host.ExecuteScript("script.js"))
    {
        std::cout << "Failed to execute script" << std::endl;
        return;
    }
    while (!host.HasQuit())
    {
        if (host.HasWork())
        {
            host.RunTasks();
        }
        else
        {
            int sleepFor = host.TimeUntilNextTask();
            auto start = std::chrono::high_resolution_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepFor));
            auto end = std::chrono::high_resolution_clock::now();
            //TODO:sleep thread until we have work to do.
            std::chrono::duration<double, std::milli> elapsed = end - start;
            host.RemoveElapsedTime(static_cast<int>(elapsed.count()));
        }
    }
}

int main()
{
    std::thread JSThread(createJSThread);
    std::cout << "Started JS thread" << std::endl;
    JSThread.join();
    return 0;
}