#include "ChakraWrappers.h"
#include "JSHost.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <memory>

using namespace std;

JSHost::JSHost()
{
    JsCreateRuntime(JsRuntimeAttributeNone, nullptr, &runtime);
    JsCreateContext(runtime, &context);
    JsSetCurrentContext(context);
    this->CreateEnviroment();
}

JSHost::~JSHost()
{
    taskQueue.clear();
    JsSetCurrentContext(JS_INVALID_REFERENCE);
    JsDisposeRuntime(runtime);
}


bool JSHost::CreateEnviroment()
{
    JsValueRef global;
    JsGetGlobalObject(&global);
    //set up a "normal" looking JS enviroment
    JsValueRef console;
    JsCreateObject(&console);

    SetCallback(global, "setTimeout", JSHost::SetTimeout, nullptr);
    SetCallback(global, "setInterval", JSHost::SetInterval, nullptr);
    SetCallback(global, "clearTimeout", JSHost::ClearTimeout, nullptr);
    SetCallback(global, "clearInterval", JSHost::ClearTimeout, nullptr);

    SetProperty(global, "console", console);
    SetCallback(console, "log", JSHost::Log, nullptr);

    //create our application binding
    JsValueRef application;
    JsCreateObject(&application);

    SetProperty(global, "application", application);
    SetCallback(application, "quit", JSHost::Quit, nullptr);
    //bind the host to the global so we can pull it off in our function calls
    JsValueRef hostObj;
    JsSetContextData(context, this);

    return true;
}

std::experimental::optional<std::string> JSHost::loadPathContents(path p)
{
    std::ifstream in(p.c_str(), std::ios::in | std::ios::binary);
    if (in)
    {
        std::stringstream contents;
        contents << in.rdbuf();
        in.close();
        return std::experimental::optional<std::string>(contents.str());
    }
    return std::experimental::optional<std::string>();
}

bool JSHost::ExecuteScript(path p)
{
    JsValueRef scriptResult = JS_INVALID_REFERENCE;
    std::experimental::optional<std::string> result = loadPathContents(p);
    if (result)
    {
        JsErrorCode err = JsRunScriptContents(*result, JS_SOURCE_CONTEXT_NONE, "", &scriptResult);
        if (err != JsNoError)
        {
            // Get error message
            JsValueRef exception;
            JsGetAndClearException(&exception);

            JsPropertyIdRef messageName;
            if (JsGetPropertyIdFromName("message", &messageName) != JsNoError)
            {
                std::cout << "failed to get error message id" << std::endl;
                return false;
            }

            JsValueRef messageValue;
            JsGetProperty(exception, messageName, &messageValue);
            std::string exMessage;

            //see chakrawrappers.h
            JsStringToString(messageValue, exMessage);
            std::cout << exMessage << std::endl;
            return false;
        }
        return true;
    }
    return false;
}

bool JSHost::AddTaskToQueue(std::shared_ptr<Task> task)
{
    task->Reset();
    taskQueue.push_back(task);
    //possible improvment: we could be a little smarter here.
    //Figure out if std::priority_queue is more appropriate.
    //or, simply make_heap only when we need to use it as a heap that is the time it needs to be a heap
    //by persisting the next wakeup time and then when we do wake, make_heap.
    //this would mean that anyone setting a timeout then quickly calling cleartimeout potentially can have 0(1)
    auto comp = [](std::shared_ptr<Task> l, std::shared_ptr<Task> r) { return l->GetDelay() < r->GetDelay(); };
    make_heap(taskQueue.begin(), taskQueue.end(), comp);
    if (task->GetDelay() == 0)
    {
        hasWork = true;
    }
    return true;
}

void JSHost::RunTasks()
{
    while (!taskQueue.empty())
    {
        std::shared_ptr<Task> task = taskQueue.back();
        if (task->GetDelay() > 0)
        {
            break;
        }
        task->Run();
        taskQueue.pop_back();
        if(task->DoesRepeat()) 
        {
            this->AddTaskToQueue(task);
        }
    } 
    hasWork = false;
}

int JSHost::TimeUntilNextTask() 
{
    if(taskQueue.size() == 0) {
        //for now sleep for 1 second before we check for more work.
        //what we actually need to do is have a signal that we check incase of nested calls to settimeout
        return 1000;
    }
    std::shared_ptr<Task> task = taskQueue.back();
    return task->GetDelay();
}

void JSHost::RemoveElapsedTime(int milliseconds) 
{
    if(taskQueue.empty())
    {
        return;
    }

    for(std::shared_ptr<Task> t : taskQueue) 
    {
        t->ReduceDelay(milliseconds);
    }
    std::shared_ptr<Task> task = taskQueue.back();
    if (task->GetDelay() < 1) 
    {
        hasWork = true;
    }
}

bool JSHost::ClearTask(int id) 
{
    auto task = std::find_if(taskQueue.begin(), taskQueue.end(), 
    [id](std::shared_ptr<Task> const& t) 
    { 
        return t->timeoutID == id;
    });
    if(task == taskQueue.end()) {
        return false;
    }
    //according to the spec we don't pull this from the queue, we simply mark it not to run'
    (*task)->ClearTimeout();
    return true;
}

JsValueRef JSHost::ClearTimeout(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
     if (isConstructCall || argumentCount < 2) {
         std::cout << "clearTimeout needs at least a function to execute" << std::endl;
         return JS_INVALID_REFERENCE;
     }
    int id = -1;
    auto host = GetObjectFromContex<JSHost*>(callee);
    if(JsNumberToInt(arguments[1], &id) == JsErrorInvalidArgument) {
        id = -1;
    }
    host->ClearTask(id);
    return JS_INVALID_REFERENCE;
}

JsValueRef JSHost::SetTimeout(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    if (isConstructCall || argumentCount < 2)
    {
        std::cout << "setTimeout needs at least a function to execute" << std::endl;
        return JS_INVALID_REFERENCE;
    }
    
    return CreateTimedTask(callee, arguments, argumentCount, false);
}

JsValueRef JSHost::SetInterval(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    if (isConstructCall || argumentCount < 2)
    {
        std::cout << "setInterval needs at least a function to execute" << std::endl;
        return JS_INVALID_REFERENCE;
    }
    
    return CreateTimedTask(callee, arguments, argumentCount, true);
}

JsValueRef JSHost::CreateTimedTask(JsValueRef callee, JsValueRef *arguments, unsigned short argumentCount, bool repeat) {
  // https://www.w3.org/TR/2011/WD-html5-20110525/timers.html#get-the-timeout 
    int delay = 0;
    JsValueRef func = arguments[1];
    //from chakrawrappers.h
    auto host = GetObjectFromContex<JSHost*>(callee);

    if(JsNumberToInt(arguments[2], &delay) == JsErrorInvalidArgument) {
        delay = 0;
    }
    
    JsValueRef timeoutID;
    std::vector<JsValueRef> extraArgs;
    if(argumentCount > 3) 
    {
        for(int i = 3; i < argumentCount; i++) 
        {   
            extraArgs.push_back(arguments[i]);
        }
    }

    std::shared_ptr<Task> t = std::make_shared<Task>(delay, repeat, func, arguments[0], extraArgs);
    JsIntToNumber(t->timeoutID, &timeoutID);
    host->AddTaskToQueue(t);
    return timeoutID;
}

JsValueRef JSHost::Quit(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    //from chakrawrappers.h
    auto host = GetObjectFromContex<JSHost*>(callee);
    host->hasQuit = true;
    return JS_INVALID_REFERENCE;
}

JsValueRef JSHost::Log(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    for (unsigned int i = 1; i < argumentCount; i++)
    {
        if (i > 1)
        {
            std::cout << " ";
        }
        JsValueRef resultJSString;
        JsConvertValueToString(arguments[i], &resultJSString);
        std::string toLog;
        //see chakrawrappers.h
        JsStringToString(resultJSString, toLog);
        std::cout << toLog;
    }
    std::cout << std::endl;
    return JS_INVALID_REFERENCE;
}