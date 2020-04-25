using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Android.App;
using Android.Content;
using Android.OS;
using Android.Runtime;
using Android.Views;
using Android.Widget;
using WebAtoms;
using V8Handle = System.IntPtr;


namespace Xamarin.Android.V8
{

    internal delegate V8Response ExternalCall(V8Response fx, V8Response thisHandle, V8Response args);

    internal delegate V8Response CLRExternalCall(V8Response thisHandle, V8Response args);

    public delegate JSValue Function(JSValue jsThis, JSValue jsArgs);

    internal delegate IntPtr ReadDebugMessage();

    internal delegate void ReadDebugMessageFromV8(
        int len,
        [MarshalAs(UnmanagedType.LPStr, SizeParamIndex = 0)]
        string char8,
        [MarshalAs(UnmanagedType.LPWStr, SizeParamIndex = 0)]
        string char16);

    internal delegate void JSContextLog(IntPtr text);

    internal delegate void JSDeallocator(IntPtr ptr);

    internal delegate void FatalErrorCallback(IntPtr location, IntPtr message);

    internal delegate void QueueTask(IntPtr task, double delay);

    internal enum NullableBool: byte
    {
        NotSet = 0,
        False = 1,
        True = 2
    }

    public class JSContext: IJSContext, IDisposable
    {

        private static object creationLock = new object();

        const string LibName = "liquidjs";

        static JSDeallocator deAllocator;
        static FatalErrorCallback fatalErrorCallback;
        static ExternalCall externalCaller;

        ReadDebugMessageFromV8 receiveDebugFromV8;
        ReadDebugMessage readDebugMessage;
        JSContextLog logger;


        public Action<string> Logger { get; set; }

        internal V8Handle context;

        public event EventHandler<ErrorEventArgs> ErrorEvent;

        public JSValue Undefined { get; }

        IJSValue IJSContext.Undefined => Undefined;

        public IJSValue Global { get; }

        public JSValue Null { get; }

        public string Stack => this.Global["Error"].CreateNewInstance()["stack"].ToString();

        public IJSValue this[string name] { get => this.Global[name]; set => this.Global[name] = value; }

        internal IJSValue WrappedSymbol { get; }

        private V8InspectorProtocol inspectorProtocol;

        /// <summary>
        /// Creates JSContext with inverse web socket proxy, this is helpful if you do not want to create
        /// server on the device. You can simply create a websocket proxy server and use that to proxy
        /// inspector protocol.
        /// 
        /// You can specify additional parameters on uri to identify connecting client.
        /// </summary>
        /// <param name="webSocketProxyUri"></param>
        public JSContext(Uri webSocketProxyUri)
            : this(V8InspectorProtocol.CreateInverseProxy(webSocketProxyUri))
        {

        }

        /// <summary>
        /// Creates new JSContext with specified web socket server port if debug is true
        /// </summary>
        /// <param name="debug"></param>
        /// <param name="webSocketServerPort"></param>
        public JSContext(bool debug = false, int webSocketServerPort = 9222)
            : this(debug  ? V8InspectorProtocol.CreateWebSocketServer(webSocketServerPort) : null )
        {

        }

        private JSContext(V8InspectorProtocol protocol = null)
        {
            inspectorProtocol = protocol;
            logger = (t) => {
                var s = Marshal.PtrToStringUTF8(t);
                Logger?.Invoke(s);
            };
            Logger = (s) => {
                System.Diagnostics.Debug.WriteLine(s);
            };


            fatalErrorCallback = (l, m) => {
                string ls = l == IntPtr.Zero  ? null : Marshal.PtrToStringAuto(l);
                string ms = m == IntPtr.Zero ? null : Marshal.PtrToStringAuto(m);
                Logger?.Invoke(ms);
                Logger?.Invoke(ls);
            };

            readDebugMessage = () => {

                string msg = AsyncHelpers.RunSync<string>(ReadDebugMessageAsync);
                return Marshal.StringToAllocatedMemoryUTF8(msg);
                
            };

            receiveDebugFromV8 = (n, c8, c16) => {
                try {
                    if (n > 0)
                    {
                        var msg = c8 ?? c16;                        
                        // Log(msg);
                        this.inspectorProtocol.SendMessage(msg);
                    }
                } catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine(ex);
                }
            };

            lock (creationLock)
            {
                if (deAllocator == null)
                {

                    deAllocator = (p) =>
                    {
                        try
                        {
                            GCHandle g = GCHandle.FromIntPtr(p);
                            if (g.IsAllocated)
                            {
                                g.Free();
                            }
                        }
                        catch (Exception ex)
                        {
                            System.Diagnostics.Debug.WriteLine(ex);
                        }
                    };

                    externalCaller = (fx, t, a) =>
                    {
                        try
                        {
                            var fxc = fx.GetContainer();
                            var gc = GCHandle.FromIntPtr(fxc.result.handle.value.refValue);
                            var ffx = (CLRExternalCall)gc.Target;
                            return ffx(t, a);
                        }
                        catch (Exception ex)
                        {
                            var msg = Marshal.StringToAllocatedMemoryUTF8(ex.ToString());
                            return new V8Response
                            {
                                type = V8ResponseType.Error,
                                result = new V8ResponseResult
                                {
                                    error = new V8Error
                                    {
                                        message = msg,
                                        stack = IntPtr.Zero
                                    }
                                }
                            };
                        }
                    };

                }


                this.context = V8Context_Create(
                    protocol != null,
                    logger: Marshal.GetFunctionPointerForDelegate(logger),
                    externalCall: Marshal.GetFunctionPointerForDelegate(externalCaller),
                    freeMemory: Marshal.GetFunctionPointerForDelegate(deAllocator),
                    debugReceiver: Marshal.GetFunctionPointerForDelegate(readDebugMessage),
                    receiveDebugFromV8: Marshal.GetFunctionPointerForDelegate(receiveDebugFromV8),
                    queueTask: IntPtr.Zero,
                    fatalErrorCallback: Marshal.GetFunctionPointerForDelegate(fatalErrorCallback));
            }
            
            this.Undefined = new JSValue(this, V8Context_CreateUndefined(context).GetContainer());

            this.Global = new JSValue(this, V8Context_GetGlobal(context).GetContainer());

            this.Null = new JSValue(this, V8Context_CreateNull(context).GetContainer());

            this.WrappedSymbol = new JSValue(this, V8Context_CreateSymbol(context, "WrappedSymbol").GetContainer());

            // Add SetTimeout...

            Dictionary<int, IDisposable> timeouts = new Dictionary<int, IDisposable>();

            int id = 0;

            this["clearTimeout"] = this.CreateFunction(1, (c, a) => {
                var tid = a[0].IntValue;
                if(timeouts.TryGetValue(tid, out var token))
                {
                    token.Dispose();
                }
                return this.Undefined;
            }, "clearTimeout");

            this["setTimeout"] = this.CreateFunction(2, (c, a) => {

                var fn = a[0];
                var timeout = a[1];
                var delay = timeout.IsUndefined ? 0 : timeout.LongValue;
                var ct = MainThread.PostTimeout(() => {
                    fn.InvokeFunction(Global);
                }, delay);

                var tid = System.Threading.Interlocked.Increment(ref id);
                return this.CreateNumber(tid);

            }, "setTimeout");

            if (protocol != null)
            {
                MainThread.InvokeOnMainThreadAsync(() => this.SetupDebugging());
            }

        }


        private async Task<string> ReadDebugMessageAsync()
        {
            lock(this)
            {
                readMessageTask = new TaskCompletionSource<string>();
            }
            var msg = await readMessageTask.Task;
            lock(this)
            {
                readMessageTask = null;
            }
            return msg;
        }

        private TaskCompletionSource<string> readMessageTask;

        private static void Log(object message)
        {
            System.Diagnostics.Debug.WriteLine(message);
        }

        private async Task SetupDebugging()
        {
            try
            {
                await inspectorProtocol.ConnectAsync((msg) =>
                {
                    lock (this)
                    {
                        if (readMessageTask != null)
                        {
                            readMessageTask.TrySetResult(msg);
                            return;
                        }
                    }

                    MainThread.BeginInvokeOnMainThread(() =>
                    {
                        try
                        {
                            // System.Diagnostics.Debug.WriteLine(msg);
                            V8Context_SendDebugMessage(context, msg.Length, msg).GetBooleanValue();
                        }
                        catch (Exception ex)
                        {
                            Log(ex);
                        }
                    });
                });
            } catch (Exception ex)
            {
                Log(ex);
            }
        }

        public IJSValue CreateObject()
        {
            return new JSValue(this, V8Context_CreateObject(context).GetContainer());
        }

        public IJSValue CreateNull()
        {
            return new JSValue(this, V8Context_CreateNull(context).GetContainer());
        }

        public IList<IJSValue> CreateArray()
        {
            var a = new JSValue(this, V8Context_CreateArray(context).GetContainer());
            return a.ToArray();
        }

        public IJSValue CreateString(string value)
        {
            return new JSValue(this, V8Context_CreateString(context, value).GetContainer());
        }

        public IJSValue CreateSymbol(string name)
        {
            return new JSValue(this, V8Context_CreateSymbol(context, name).GetContainer());
        }

        public IJSValue CreateNumber(double value)
        {
            return new JSValue(this, V8Context_CreateNumber(context, value).GetContainer());
        }

        public IJSValue CreateBoolean(bool value)
        {
            return new JSValue(this, V8Context_CreateBoolean(context, value).GetContainer());
        }

        public IJSValue CreateDate(DateTime value)
        {
            return new JSValue(this, V8Context_CreateDate(context, value.ToJSTime()).GetContainer());
        }

        public IJSValue CreateFunction(int args, Func<IJSContext, IJSValue[], IJSValue> fx, string debugDescription)
        {
            CLRExternalCall efx = (t, a) => {
                try
                {
                    var tjs = new JSValue(this, t.GetContainer());

                    var targs = new JSValue(this, a.GetContainer());
                    var len = targs.Length;
                    var al = new IJSValue[len];
                    for (int i = 0; i < len; i++)
                    {
                        al[i] = targs[i];
                    }
                    var r = fx(this, al) as JSValue;
                    return new V8Response
                    {
                        type = V8ResponseType.Handle,
                        result = new V8ResponseResult
                        {
                            handle = new V8HandleContainer
                            {
                                handle = r?.GetHandle() ?? IntPtr.Zero
                            }
                        }
                    };
                } catch (Exception ex)
                {
                    IntPtr msg = Marshal.StringToAllocatedMemoryUTF8(ex.ToString());
                    // IntPtr stack = Marshal.StringToAllocatedMemoryUTF8(ex.StackTrace);
                    return new V8Response {
                        type = V8ResponseType.Error,
                        result = new V8ResponseResult
                        {
                            error = new V8Error
                            {
                                message = msg,
                                stack = IntPtr.Zero
                            }
                        }
                    };
                }
            };

            var gfx = GCHandle.Alloc(efx);
            var ptr = GCHandle.ToIntPtr(gfx);
            var c = V8Context_CreateFunction(context, ptr, debugDescription).GetContainer();
            return new JSValue(this, c);
        }

        public IJSValue Evaluate(string script, string location = null)
        {
            location = location ?? "vm";
            var c = V8Context_Evaluate(
                context, 
                script.Length, 
                script, 
                location.Length,
                location).GetContainer();
            return new JSValue(this, c);
        }

        public IJSValue Convert(object value)
        {
            if (value == null)
            {
                return this.Undefined;
            }
            if (value is IJSValue jsv)
            {
                return jsv;
            }
            if (value is IJSService jvs)
            {
                // build...
                return JSService.Create(this, jvs);
            }
            if (value is string s)
                return this.CreateString(s);

            if (value is int i)
                return this.CreateNumber(i);

            if (value is float f)
                return this.CreateNumber(f);

            if (value is double d)
                return this.CreateNumber(d);
            if (value is decimal dec)
                return this.CreateNumber((double)dec);

            if (value is bool b)
                return this.CreateBoolean(b);
            
            if (value is AtomEnumerable en)
            {
                return en.array;
            }

            if (value is DateTime dt)
            {
                return this.CreateDate(dt);
            }
            
            if (value is Task<IJSValue> task)
            {
                return this.CreatePromiseWithResult(task);
            }


            var wgc = GCHandle.Alloc(value);
            var wgcPtr = GCHandle.ToIntPtr(wgc);
            var wrapped = new JSValue(this, V8Context_Wrap(context, wgcPtr ).GetContainer());
            var w = new JSValue(this, V8Context_CreateObject(context).GetContainer());

            if (!(value is IJSContext))
            {
                w.DefineProperty("expand", new JSPropertyDescriptor
                {
                    Enumerable = true,
                    Configurable = true,
                    Get = CreateFunction(0, (c, a) =>
                    {
                        return this.Serialize(value, SerializationMode.Reference);
                    }, "Expand")
                });

                w["appendChild"] = this.CreateFunction(1, (c, p) =>
                {
                    this["bridge"].InvokeMethod("appendChild", new IJSValue[] { w, p[0] });
                    return this.Undefined;
                }, "appendChild");

                w["dispatchEvent"] = this.CreateFunction(1, (c, p) =>
                {
                    var first = p[0];
                    this["bridge"].InvokeMethod("dispatchEvent", new IJSValue[] { w, first });
                    return first;
                }, "dispatchEvent");

            }
            w[WrappedSymbol] = wrapped;
            return w;
        }

        public bool HasProperty(string name)
        {
            return this.Global.HasProperty(name);
        }

        public bool DeleteProperty(string name)
        {
            return this.Global.DeleteProperty(name);
        }

        public void RunOnUIThread(Func<Task> task)
        {
            MainThread.BeginInvokeOnMainThread(async () => {
                try
                {
                    await task();
                }
                catch (Exception ex)
                {
                    Logger?.Invoke(ex.ToString());
                }
            });
        }

        ~JSContext()
        {
            if (context == IntPtr.Zero)
                return;
            Dispose();
        }

        [DllImport(LibName)]
        internal extern static V8Handle V8Context_Create(
            bool debug, 
            IntPtr logger, 
            IntPtr externalCall,
            IntPtr freeMemory,
            IntPtr debugReceiver,
            IntPtr receiveDebugFromV8,
            IntPtr queueTask,
            IntPtr fatalErrorCallback);

        [DllImport(LibName)]
        internal extern static void V8Context_Dispose(V8Handle context);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateObject(V8Handle context);


        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateNull(V8Handle context);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateUndefined(V8Handle context);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateArray(V8Handle context);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateBoolean(V8Handle context, bool value);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateNumber(V8Handle context, double value);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateDate(V8Handle context, long value);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateString(V8Handle context, 
            [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateSymbol(V8Handle context,
            [MarshalAs(UnmanagedType.LPUTF8Str)] string name);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_GetGlobal(V8Handle context); 
        
        [DllImport(LibName)]
        internal extern static V8Response V8Context_NewInstance(V8Handle context, 
            V8Handle target, int len,
            [MarshalAs(UnmanagedType.LPArray)]
            V8Handle[] args);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_InvokeMethod(V8Handle context,
            V8Handle target,
            [MarshalAs(UnmanagedType.LPUTF8Str)]
            string name,
            int len,
            [MarshalAs(UnmanagedType.LPArray)]
            V8Handle[] args);


        [DllImport(LibName)]
        internal extern static V8Response V8Context_InvokeFunction(V8Handle context,
            V8Handle target,
            V8Handle thisValue,
            int len,
            [MarshalAs(UnmanagedType.LPArray)]
            V8Handle[] args);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_DefineProperty(V8Handle context,
            V8Handle target,
            [MarshalAs(UnmanagedType.LPUTF8Str)]
            string name,
            [MarshalAs(UnmanagedType.SysInt)]
            NullableBool configurable,
            [MarshalAs(UnmanagedType.SysInt)]
            NullableBool enumerable,
            [MarshalAs(UnmanagedType.SysInt)]
            NullableBool writable,
            IntPtr get,
            IntPtr set,
            IntPtr value
        );


        [DllImport(LibName)]
        internal extern static V8Response V8Context_GetArrayLength(
            V8Handle context,
            IntPtr handle);


        [DllImport(LibName)]
        internal extern static V8Response V8Context_HasProperty(
            V8Handle context,
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPUTF8Str)]
            string name);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_Has(
            V8Handle context,
            IntPtr handle,
            IntPtr symbol);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_DeleteProperty(
            V8Handle context,
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPUTF8Str)]
            string name);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_Get(
            V8Handle context,
            IntPtr handle,
            IntPtr name);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_Set(
            V8Handle context,
            IntPtr handle,
            IntPtr name,
            IntPtr value);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_GetProperty(
            V8Handle context, 
            IntPtr handle, 
            [MarshalAs(UnmanagedType.LPUTF8Str)]
            string name);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_SetProperty(
            V8Handle context,
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPUTF8Str)]
            string name,
            IntPtr value);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_GetPropertyAt(V8Handle context, IntPtr handle, int index);


        [DllImport(LibName)]
        internal extern static V8Response V8Context_SetPropertyAt(
            V8Handle context,
            IntPtr handle,
            int index,
            IntPtr value);

        [DllImport(LibName)]
        internal extern static void V8Context_PostTask(
            IntPtr handle
            );


        [DllImport(LibName)]
        internal extern static V8Response V8Context_SendDebugMessage(
            V8Handle context,
            int len,
            [MarshalAs(UnmanagedType.LPWStr)]
            string message
            );

        [DllImport(LibName)]
        internal extern static V8Response V8Context_ToString(
            V8Handle context,
            IntPtr handle);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_Equals(V8Handle context, IntPtr left, IntPtr right);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_Wrap(
            V8Handle context,
            IntPtr handle);


        [DllImport(LibName)]
        internal extern static void V8Context_Release(V8Response r);


        [DllImport(LibName)]
        internal extern static V8Response V8Context_ReleaseHandle(IntPtr context, IntPtr r);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateFunction(
            V8Handle context, IntPtr function, [MarshalAs(UnmanagedType.LPUTF8Str)]string name);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_Evaluate(
            V8Handle context,
            int lenScript,
            [MarshalAs(UnmanagedType.LPWStr)]
            string script,
            int lenLocation,
            [MarshalAs(UnmanagedType.LPWStr)]
            string location);

        public void Dispose()
        {
            if (context == IntPtr.Zero)
                return;
            V8Context_Dispose(context);
            context = IntPtr.Zero;
        }
    }
}