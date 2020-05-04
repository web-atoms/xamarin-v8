[![NuGet](https://img.shields.io/nuget/v/Xamarin.Android.V8.svg?label=NuGet)](https://www.nuget.org/packages/Xamarin.Android.V8)

# Xamarin V8 Bindings
V8 Bindings for Xamarin for Android

# Limitation

Currently ARM64 is not supported as something is broken in mono interop, till the time it is resolved, you can remove `arm64-v8a` ABI from your app. This limitation is temporary till we resolve the issue.

# NuGet
```xml
<PackageReference Include="Xamarin.Android.V8" Version="1.4.79" />
```
# Inspector Protocol Port
Visual Studio > Tools > Android > Android Adb Command Prompt
```
adb forward tcp:9222 tcp:9222
```

If you want to change the default 9222 port, you can specify in the parameters.



# Create Context

```c#
using(var context = new JSContext( /*Enable Debugging*/ true)) {

  // you can connect to dev tools by visiting url
  // devtools://devtools/bundled/inspector.html?experiments=true&v8only=true&ws=127.0.0.1:9222/backend
  
  

}
```

# Create New Global Function
```c#
context["printf"] = context.CreateFunction(0, (c, a) => {
  // first parameter is context isself
  // second parameter is an array as IJSValue
  System.Diagnostics.Debug.WriteLine(a[0].ToString());
  return c.Undefined;
});

// JS : printf("a");
```

# Evaluate Script with Location
```c#
// script location is useful for debugging
context.Evaluate(scriptText, scriptLocation);
```

# Navigate Objects

```c#
   // Object.create() JavaScript Equivalent in c#
   var obj = context["Object"].InvokeMethod("create");
```

# Convert Native Objects
As V8 Engine has its own representation of native types, you need to create them in order to pass them as parameters for JavaScript methods.

```c#
   var jsString = context.CreateString("TEXT");
   var jsNumber = context.CreateNumber(100);
   
   context["console"].InvokeMethod("log", jsString);
   context["console"].InvokeMethod("log", jsNumber);
```

# Evaluate Template

```c#

   // clr object
   var s = "Akash";

   context.EvaluateTemplate($"console.log({s})");

```

This method serializes every clr object input as IJSValue by doing `Wrap` serialization. You can however serialize and then send object as parameter to templated string. It will store all object in global and evaluate JavaScript, you do not have to worry about the escape as each input is serialized and they are substituted as parameter to placeholders.

# Serialize C# Object

When you use method `context.Convert` method to automatically create native JS values from native types, it will only wrap C# custom object, you cannot call any method or access property from JavaScript on wrapped object. This is done to improve performance. So when you pass C# objects in and out, engine will not create methods and properties on them.

In order to access methods and properties of C# object, you have to serialize them.

```c#

   // you can access all properties, no methods
   var jsDictObject = context.Serialize( customClrObject , SerializationMode.Copy);
   
   // you can access all properties and invoke method as well
   var jsClrObject = context.Serialize( customClrObject , SerializationMode.Reference);

```

## Serialization Modes

### Copy
This method will create a deep copy of CLR Object as dictionary which you can easily access inside JavaScript code. This method will fail if there are self referencing objects in the object graph. This limitation may be removed in future, but right now it will throw an exception.

This method is also very slow as deep copy operation will take more time.

Deserialization will also be slow as it will completely construct new object with all properties.

### Reference
Keeps reference along with serialization, every property is serialized as getter/setter, upon deserialization, same object will be returned.

This method is useful for self referencing objects, but this may cause memory leak if you keep reference in JavaScript and JavaScript garbage collector fails to dispose object.

Deserialization is faster as it simply returns referenced object.

### WeakReference
Same as Reference but it only keeps weak reference, you will get object disposed if you try to access object in JavaScript and it is disposed in CLR. CLR is very aggressive while disposing objects, so this may not work if you do not keep reference in CLR. This is also recommended method as it will avoid memory leaks.

### Wrap
This is default serialization method for any object. Object will simply be wrapped and no methods/properties are exposed.




# Resources
1. https://kerry.lothrop.de/c-libraries/
2. https://devblogs.microsoft.com/cppblog/developing-xamarin-android-native-applications/
3. https://devblogs.microsoft.com/wp-content/uploads/sites/9/2019/02/XamarinNativeExample.zip
4. https://github.com/rjamesnw/v8dotnet/blob/master/Source/V8.NET-Proxy/ProxyTypes.h
5. https://devblogs.microsoft.com/cppblog/android-and-ios-development-with-c-in-visual-studio/
6. https://github.com/lothrop/XamarinNative
7. https://github.com/xamcat/mobcat-samples/tree/master/cpp_with_xamarin
8. https://stackoverflow.com/questions/31541451/create-shared-library-from-cpp-files-and-static-library-with-g
9. https://hyperandroid.com/2020/02/12/compile-v8-arm-arm64-ia32/
10. https://hyperandroid.com/2020/02/12/android-v8-embedding-guide/
11. https://hyperandroid.com/2020/02/12/v8-inspector-from-an-embedder-standpoint/
