#include "ChakraCore.h"
#include <cstring>
#include <cstdlib>
#include <string>
#include <functional>

///a collection of function that make life easier using c++ types mainly std::string
#pragma once

#if !defined(_WIN32)  
inline JsErrorCode JsGetPropertyIdFromName(const char* str, JsPropertyIdRef *Id)
{
    return JsCreatePropertyId(str, strlen(str), Id);
}
#endif

JsErrorCode JsRunScriptContents(const std::string script, JsSourceContext sourceContext, std::string sourceUrl, JsValueRef* result) {
    JsValueRef sourceLocation;
    JsValueRef scriptSource;
    JsCreateString(sourceUrl.c_str(), sourceUrl.length(), &sourceLocation);
    JsCreateExternalArrayBuffer((void *)script.c_str(), script.length(),  nullptr, nullptr, &scriptSource);
    return JsRun(scriptSource, sourceContext, sourceLocation, JsParseScriptAttributeNone, result);
}

void SetProperty(JsValueRef object, const std::string& propertyName, JsValueRef property) 
{
    JsPropertyIdRef propertyId;
	JsCreatePropertyId(propertyName.c_str(), propertyName.length(), &propertyId);
	JsSetProperty(object, propertyId, property, true);
}

void SetCallback(JsValueRef object, const std::string& propertyName, JsNativeFunction callback, void *callbackState)
{
	JsPropertyIdRef propertyId;
	JsCreatePropertyId(propertyName.c_str(), propertyName.length(), &propertyId);
	JsValueRef function;
	JsCreateFunction(callback, callbackState, &function);
	JsSetProperty(object, propertyId, function, true);
}

JsErrorCode JsCopyTo(JsRef propertyId, std::string& result, std::function<JsErrorCode(JsValueRef, char *, size_t, size_t*)> copyFunc) 
{
    size_t length = 0;
    size_t bufferLength = 0;
    copyFunc(propertyId, nullptr, 0, &length);
    char* s = (char *)malloc(length);
    JsErrorCode error = copyFunc(propertyId, s, length, &bufferLength);
    result = std::string(s, length);
    free(s);
    return error;
}

JsErrorCode JsStringToString(JsValueRef jsString, std::string& result) {
    return JsCopyTo(jsString, result, JsCopyString);
}

JsErrorCode JsCreateStringFromString(std::string str, JsValueRef* result) {
    return JsCreateString(str.c_str(), str.length(), result);
}