#include "ChakraCore.h"
#include "task.h"

#include <vector>
#include <experimental/optional>
#include <string>
#include <iostream>
#include <memory>
#pragma once

typedef std::string path;

class JSHost
{
  public:
    JSHost();
    ~JSHost();
    bool ExecuteScript(path p);
    bool HasQuit() { return hasQuit; }
    bool AddTaskToQueue(std::shared_ptr<Task> task);
    std::vector<std::shared_ptr<Task>> taskQueue;
    void RunTasks();
    int TimeUntilNextTask();
    void RemoveElapsedTime(int milliseconds);
    bool HasWork() { return hasWork; }
    bool ClearTask(int id);
  private:
    std::experimental::optional<std::string> loadPathContents(path p);
    static JsValueRef Log(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef Quit(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef SetTimeout(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef SetInterval(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef ClearTimeout(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
  
    static JsValueRef CreateTimedTask(JsValueRef callee, JsValueRef *arguments, unsigned short argumentCount, bool repeat);
  


    bool CreateEnviroment();
    JsRuntimeHandle runtime;
    JsContextRef context;
    bool hasQuit = false;
    bool hasWork = false;
};
