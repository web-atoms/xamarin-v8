using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
//using Android.App;
//using Android.Content;
//using Android.OS;
//using Android.Runtime;
//using Android.Views;
//using Android.Widget;
using WebAtoms;
using V8Handle = System.IntPtr;

using WebAtoms.V8Sharp;


namespace Xamarin.Android.V8
{

    internal delegate V8Response ExternalCall(V8Response fx, V8Response thisHandle, V8Response args);

    internal delegate V8Response CLRExternalCall(V8Response thisHandle, V8Response args);

    public delegate JSValue Function(JSValue jsThis, JSValue jsArgs);

    internal delegate Utf16IntPtr ReadDebugMessage();

    internal delegate void ReadDebugMessageFromV8(
        int len,
        [MarshalAs(UnmanagedType.LPStr, SizeParamIndex = 0)]
        string char8,
        [MarshalAs(UnmanagedType.LPWStr, SizeParamIndex = 0)]
        string char16);

    internal delegate void JSContextLog(IntPtr text, int length);

    internal delegate void JSFreeMemory(IntPtr ptr);

    internal delegate Utf16Value JSAllocateString(int len);

    internal delegate IntPtr JSAllocateMemory(int len);

    internal delegate void FatalErrorCallback(IntPtr location, IntPtr message);

    internal delegate void BreakPauseOn(bool boolSwitch);


    internal enum NullableBool: byte
    {
        NotSet = 0,
        False = 1,
        True = 2
    }

    internal class ReadMessageLock
    {

        private AutoResetEvent wait;
        private string value;
        public ReadMessageLock()
        {

        }

        public string Read()
        {
            AutoResetEvent w;
            lock(this)
            {
                w = wait;
            }
            if (w == null)
                return "{}";
            
            w.WaitOne();
            w.Reset();
            return value;
        }

        public bool SendIfWaiting(string message)
        {
            lock (this)
            {
                if(wait == null)
                    return false;
                value = message;
                wait.Set();
            }
            return true;
        }
        public void Turn(bool b)
        {
            lock(this)
            {
                if (b)
                {
                    wait = new AutoResetEvent(false);
                } else
                {
                    wait = null;
                }
            }
        }
    }
    public class JSContext : IJSContext, IDisposable
    {

        private static readonly object creationLock = new object();

#if __IOS__
        const string LibName = "__Internal";
#else
        const string LibName = "liquidjs";
#endif
        static JSFreeMemory freeHandle;
        static JSFreeMemory freeMemory;
        static FatalErrorCallback fatalErrorCallback;
        static ExternalCall externalCaller;
        static JSAllocateMemory allocateMemory;
        static JSAllocateString allocateString;

        readonly ReadDebugMessageFromV8 receiveDebugFromV8;
        readonly ReadDebugMessage readDebugMessage;
        readonly JSContextLog logger;
        readonly BreakPauseOn breakPauseOn;


        public Action<string> Logger { get; set; }

        internal V8ContextHandle context;

        public event EventHandler<ErrorEventArgs> ErrorEvent;

        public JSValue Undefined { get; }

        IJSValue IJSContext.Undefined => Undefined;

        public IJSValue Global { get; }

        public JSValue Null { get; }

        public JSValue True { get; }

        public JSValue False { get; }

        IJSValue IJSContext.Null => Null;

        IJSValue IJSContext.True => True;

        IJSValue IJSContext.False => False;

        public string Stack => this.Global["Error"].CreateNewInstance()["stack"].ToString();

        public IJSValue this[string name] { get => this.Global[name]; set => this.Global[name] = value; }

        public IJSValue this[IJSValue name] { get => this.Global[name]; set => this.Global[name] = value; }

        internal IJSValue WrappedSymbol { get; }

        private (IJSValue appendChild, IJSValue addEventListener, IJSValue dispatchEvent) ElementWrapper;

        // private IJSValue _elementWrapper;
        //private IJSValue ElementWrapper => 
        //    _elementWrapper ?? (_elementWrapper = this.Evaluate(
        //        @"(function () {
        //            var bridge = global.bridge ? global.bridge : null;
        //            function ElementWrapper () {
        //            }
        //            if (bridge) {
        //                ElementWrapper.prototype.appendChild = function(e) { return bridge.appendChild(this,e);  };
        //                ElementWrapper.prototype.dispatchEvent = function(e) { return bridge.dispatchEvent(this,e);  };
        //                Object.defineProperty(ElementWrapper.prototype, 'expand', {
        //                    get: function() { return bridge.describe(this); }, configurable: true, enumerable: true
        //                });
        //            }
        //            return ElementWrapper;
        //        })()"));

        private readonly V8InspectorProtocol inspectorProtocol;

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

        private readonly ReadMessageLock readLock = new ReadMessageLock();

        private JSContext(V8InspectorProtocol protocol = null)
        {
            inspectorProtocol = protocol;
            logger = (t, l) => {
                var s = t.ToUtf16String(l);
                Logger?.Invoke(s);
            };
            Logger = (s) => {
                System.Diagnostics.Debug.WriteLine(s);
            };


            readDebugMessage = () => readLock.Read();

            breakPauseOn = (b) => readLock.Turn(b);

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
                if (freeHandle == null)
                {

                    fatalErrorCallback = (l, m) => {
                        string ls = l == IntPtr.Zero ? null : Marshal.PtrToStringAuto(l);
                        string ms = m == IntPtr.Zero ? null : Marshal.PtrToStringAuto(m);
                        // Logger?.Invoke(ms);
                        // Logger?.Invoke(ls);

                        System.Diagnostics.Debug.WriteLine(ms);
                        System.Diagnostics.Debug.WriteLine(ls);
                    };


                    allocateMemory = (n) =>
                    {
                        IntPtr m = Marshal.AllocHGlobal(n);
                        return m;
                    };

                    allocateString = (n) => {
                        var s = new String(' ', n);
                        Utf16Value v = s;
                        return v;
                    };

                    freeMemory = (n) =>
                    {
                        Marshal.FreeHGlobal(n);
                    };

                    freeHandle = (p) =>
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
                            var fxc = fx;
                            var gc = GCHandle.FromIntPtr(fxc.result.refValue);
                            var ffx = (CLRExternalCall)gc.Target;
                            return ffx(t, a);
                        }
                        catch (Exception ex)
                        {
                            return ex;
                        }
                    };

                }


                this.context = V8Context_Create(
                    protocol != null,
                    new CLREnv
                    {
                        allocateMemory = Marshal.GetFunctionPointerForDelegate(allocateMemory),
                        allocateString = Marshal.GetFunctionPointerForDelegate(allocateString),
                        freeMemory = Marshal.GetFunctionPointerForDelegate(freeMemory),

                        freeHandle = Marshal.GetFunctionPointerForDelegate(freeHandle),
                        externalCall = Marshal.GetFunctionPointerForDelegate(externalCaller),

                        logger = Marshal.GetFunctionPointerForDelegate(logger),
                        WaitForDebugMessageFromProtocol = Marshal.GetFunctionPointerForDelegate(readDebugMessage),
                        SendDebugMessageToProtocol = Marshal.GetFunctionPointerForDelegate(receiveDebugFromV8),
                        fatalErrorCallback = Marshal.GetFunctionPointerForDelegate(fatalErrorCallback),

                        breakPauseOn = Marshal.GetFunctionPointerForDelegate(breakPauseOn)
                    });
            }
            
            this.Undefined = new JSValue(this, V8Context_CreateUndefined(context));

            this.Global = new JSValue(this, V8Context_GetGlobal(context));

            this.Null = new JSValue(this, V8Context_CreateNull(context));

            this.True = new JSValue(this, V8Context_CreateBoolean(context, true));

            this.False = new JSValue(this, V8Context_CreateBoolean(context, false));

            this.WrappedSymbol = new JSValue(this, V8Context_CreateSymbol(context, "WrappedSymbol"));

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

                    if (readLock.SendIfWaiting(msg))
                        return;

                    MainThread.BeginInvokeOnMainThread(() =>
                    {
                        try
                        {
                            // System.Diagnostics.Debug.WriteLine(msg);
                            V8Context_SendDebugMessage(context, msg).GetBooleanValue();
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
            return new JSValue(this, V8Context_CreateObject(context));
        }

        public IJSValue CreateNull()
        {
            return new JSValue(this, V8Context_CreateNull(context));
        }

        public IJSArray CreateArray()
        {
            var a = new JSValue(this, V8Context_CreateArray(context));
            return new AtomEnumerable(a);
        }

        public IJSValue CreateString(string value)
        {
            if (value == null)
                return Null;
            return new JSValue(this, V8Context_CreateString(context, value));
        }

        public IJSValue CreateSymbol(string name)
        {
            return new JSValue(this, V8Context_CreateSymbol(context, name));
        }

        public IJSValue CreateNumber(double value)
        {
            return new JSValue(this, V8Context_CreateNumber(context, value));
        }

        public IJSValue CreateDate(DateTime value)
        {
            return new JSValue(this, V8Context_CreateDate(context, value.ToJSTime()));
        }

        public IJSValue CreateBoundFunction(int args, WJSBoundFunction fx, string debugDescription)
        {
            CLRExternalCall efx = (t, a) => {
                try
                {
                    var tjs = new JSValue(this, t);

                    var targs = a.ToJSValueArray(this);
                    var r = fx(this, tjs, targs) as JSValue;
                    return new V8Response
                    {
                        Type = V8HandleType.Object,
                        address = r?.GetHandle() ?? IntPtr.Zero
                    };
                }
                catch (Exception ex)
                {
                    return ex;
                }
            };

            var gfx = GCHandle.Alloc(efx);
            var ptr = GCHandle.ToIntPtr(gfx);
            var fxPtr = Marshal.GetFunctionPointerForDelegate(efx);
            var c = V8Context_CreateFunction(context, fxPtr, ptr, debugDescription);
            return new JSValue(this, c);
        }

        public IJSValue CreateFunction(int args, Func<IJSContext, IList<IJSValue>, IJSValue> fx, string debugDescription)
        {
            CLRExternalCall efx = (t, a) => {
                try
                {
                    var tjs = new JSValue(this, t);

                    var targs = a.ToJSValueArray(this);
                    var r = fx(this, targs) as JSValue;
                    return new V8Response
                    {
                        Type = V8HandleType.Object,
                        address = r?.GetHandle() ?? IntPtr.Zero
                    };
                } catch (Exception ex)
                {
                    return ex;
                }
            };

            var gfx = GCHandle.Alloc(efx);
            var ptr = GCHandle.ToIntPtr(gfx);
            var fxPtr = Marshal.GetFunctionPointerForDelegate(efx);
            var c = V8Context_CreateFunction(context, fxPtr, ptr, debugDescription);
            return new JSValue(this, c);
        }

        public IJSValue Evaluate(string script, string location = null)
        {
            location = location ?? "vm";
            var c = V8Context_Evaluate(
                context, 
                script, 
                location);
            return new JSValue(this, c);
        }

        public IJSValue Wrap(object value)
        {
            var wgc = GCHandle.Alloc(value);
            var wgcPtr = GCHandle.ToIntPtr(wgc);
            var wrapped = new JSValue(this, V8Context_Wrap(context, wgcPtr));
            IJSValue w;
            if (value is IJSContext)
            {
                w = Global;
            } else
            {
                w = CreateObject();
                //if (ElementWrapper.appendChild == null)
                //{
                //    ElementWrapper.appendChild = this.Evaluate("(function(e) { return bridge.appendChild(this, e); })");
                //    ElementWrapper.dispatchEvent = this.Evaluate("(function(e) { return bridge.dispatchEvent(this, e); })");
                //    ElementWrapper.addEventListener = this.Evaluate("(function(e) { return bridge.addEventHandler(this, e); })");
                //}
                //w["appendChild"] = ElementWrapper.appendChild;
                //w["dispatchEvent"] = ElementWrapper.dispatchEvent;
                //w["addEventListener"] = ElementWrapper.addEventListener;
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
            if (context.IsDisposed)
                return;
            Dispose();
        }

        public static IntPtr CreateGlobalString(string name)
        {
            Utf16Value n = name;
            var handle = V8Context_CreateGlobalString(n);
            return handle;
        }


        [DllImport(LibName)]
        internal extern static IntPtr V8Context_CreateGlobalString(
            [MarshalAs(UnmanagedType.LPStruct)]
            Utf16Value name);

        [DllImport(LibName)]
        internal extern static V8Handle V8Context_Create(
            bool debug, 
            [MarshalAs(UnmanagedType.LPStruct)]
            CLREnv env
            );

        [DllImport(LibName, EntryPoint= nameof(V8Context_Dispose))]
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
            [MarshalAs(UnmanagedType.LPStruct)] Utf16Value value);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateSymbol(V8Handle context,
            [MarshalAs(UnmanagedType.LPStruct)] Utf16Value name);

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
            [MarshalAs(UnmanagedType.LPStruct)]
            Utf16Value name,
            int len,
            [MarshalAs(UnmanagedType.LPArray)]
            V8Handle[] args);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_InvokeMethodHandle(V8Handle context,
            V8Handle target,
            V8Handle name,
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
        internal extern static V8Response V8Context_IsInstanceOf(V8Handle context, V8Handle target, V8Handle jsClass);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_DefineProperty(V8Handle context,
            V8Handle target,
            [MarshalAs(UnmanagedType.LPStruct)]
            Utf16Value name,
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
            [MarshalAs(UnmanagedType.LPStruct)]
            Utf16Value name);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_HasPropertyHandle(
            V8Handle context,
            IntPtr handle,
            V8Handle name);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_Has(
            V8Handle context,
            IntPtr handle,
            IntPtr symbol);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_DeleteProperty(
            V8Handle context,
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStruct)]
            Utf16Value name);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_DeletePropertyHandle(
            V8Handle context,
            IntPtr handle,
            V8Handle name);

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
            [MarshalAs(UnmanagedType.LPStruct)]
            Utf16Value name);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_SetProperty(
            V8Handle context,
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStruct)]
            Utf16Value name,
            IntPtr value);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_GetPropertyHandle(
            V8Handle context,
            IntPtr handle,
            IntPtr name);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_SetPropertyHandle(
            V8Handle context,
            IntPtr handle,
            IntPtr name,
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
            [MarshalAs(UnmanagedType.LPStruct)]
            Utf16Value message
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
            V8Handle context, IntPtr function, IntPtr handle, [MarshalAs(UnmanagedType.LPStruct)]Utf16Value name);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_Evaluate(
            V8Handle context,
            [MarshalAs(UnmanagedType.LPStruct)]
            Utf16Value script,
            [MarshalAs(UnmanagedType.LPStruct)] 
            Utf16Value location);

        public void Dispose()
        {
            if (context.IsDisposed)
                return;
            V8Context_Dispose(context);
            context.Clear();
        }
    }
}