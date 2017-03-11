#include "ChakraCore.h"
#include <vector>
#include <experimental/optional>
#include <string>
#include <atomic>
#include <iostream>
#include <memory>
#pragma once

typedef std::string path;

//TODO(jared): New file for this.
class Task
{
  public:
    Task(int delay_, JsValueRef _func, JsValueRef thisArg) : delay(delay_), func(_func), timeoutID(++ID)
    {
        args[0] = thisArg;
        JsAddRef(func, nullptr);
        JsAddRef(args[0], nullptr);
    }
    int timeoutID;
    int GetDelay() { return delay; };
    void ReduceDelay(int milliseconds) { delay -= milliseconds; }
    void Run()
    {
        JsValueRef ret = JS_INVALID_REFERENCE;
        JsCallFunction(func, args, 1, &ret);
    }
    ~Task()
    {
        JsRelease(func, nullptr);
        JsRelease(args[0], nullptr);
    }

  private:
    int delay;
    JsValueRef func;
    static std::atomic_int ID;
    JsValueRef args[1];
};

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
  private:
    std::experimental::optional<std::string> loadPathContents(path p);
    static JsValueRef Log(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef Quit(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef SetTimeout(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JSHost *GetHostForContext(JsValueRef object);
    bool CreateEnviroment();
    JsRuntimeHandle runtime;
    JsContextRef context;
    bool hasQuit = false;
    bool hasWork = false;
};
