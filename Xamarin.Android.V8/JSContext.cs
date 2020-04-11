using System;
using System.Collections.Generic;
using System.Linq;
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

    internal delegate void JSContextLog(IntPtr text); 

    internal enum NullableBool: byte
    {
        NotSet = 0,
        False = 1,
        True = 2
    }

    public class JSContext: IJSContext
    {

        private static object creationLock = new object();

        const string LibName = "liquidjs";

        static IntPtr deAllocator;
        static IntPtr logger;
        static IntPtr externalCaller;

        public Action<string> Logger { get; set; }

        internal V8Handle context;

        public event EventHandler<ErrorEventArgs> ErrorEvent;

        public IJSValue Undefined { get; }

        public IJSValue Global { get; }

        public IJSValue Null { get; }

        public string Stack => throw new NotImplementedException();

        public IJSValue this[string name] { get => this.Global[name]; set => this.Global[name] = value; }

        internal IJSValue WrappedSymbol { get; }

        public JSContext(bool debug = false)
        {
            lock (creationLock)
            {
                if (deAllocator == IntPtr.Zero)
                {
                    JSContextLog _logger = (t) => {
                        var s = Marshal.PtrToStringUTF8(t);
                        Logger?.Invoke(s);
                    };
                    logger = Marshal.GetFunctionPointerForDelegate(_logger);
                    Action<IntPtr> deallocator = (p) => GCHandle.FromIntPtr(p).Free();
                    deAllocator = Marshal.GetFunctionPointerForDelegate(deallocator);

                    ExternalCall ec = (fx, t, a) => {
                        try
                        {
                            var fxc = fx.GetContainer();
                            var gc = GCHandle.FromIntPtr(fxc.value.refValue);
                            Func<V8Response, V8Response, V8Response> ffx = (Func<V8Response, V8Response, V8Response>)gc.Target;
                            return ffx(t, a);
                        }
                        catch (Exception ex)
                        {
                            var msg = Marshal.StringToAllocatedMemoryUTF8(ex.ToString());
                            return new V8Response
                            {
                                type = V8ResponseType.Error,
                                error = new V8Error
                                {
                                    message = msg,
                                    stack = IntPtr.Zero
                                }
                            };
                        }
                    };

                    externalCaller = Marshal.GetFunctionPointerForDelegate(ec);

                }
                this.context = V8Context_Create(debug, logger, externalCaller, deAllocator);
            }

            if (Logger == null)
            {
                Logger = (s) => {
                    System.Diagnostics.Debug.WriteLine(s);
                };
            }

            this.Undefined = new JSValue(this, V8Context_CreateUndefined(context).GetContainer());

            this.Global = new JSValue(this, V8Context_GetGlobal(context).GetContainer());

            this.Null = new JSValue(this, V8Context_CreateNull(context).GetContainer());

            this.WrappedSymbol = new JSValue(this, V8Context_CreateSymbol(context, "WrappedSymbol").GetContainer());
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
            Func<V8Response, V8Response, V8Response> efx = (t, a) => {
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
                    return new V8Response {
                        type = V8ResponseType.Handle,
                        handle = new V8HandleContainer {
                            handle = r?.Detach() ?? IntPtr.Zero
                        }
                    };
                } catch (Exception ex)
                {
                    IntPtr msg = Marshal.StringToAllocatedMemoryUTF8(ex.ToString());
                    // IntPtr stack = Marshal.StringToAllocatedMemoryUTF8(ex.StackTrace);
                    return new V8Response {
                        type = V8ResponseType.Error,
                        error = new V8Error {
                            message = msg,
                            stack = IntPtr.Zero
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
            var c = V8Context_Evaluate(context, script, location ?? "vm" ).GetContainer();
            return new JSValue(this, c);
        }

        public IJSValue Convert(object value)
        {
            if (value == null)
            {
                return this.Null;
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
                return this.ToPromise(task);
            }


            var wgc = GCHandle.Alloc(value, GCHandleType.Pinned);
            var wrapped = new JSValue(this, V8Context_Wrap(context, wgc.AddrOfPinnedObject()).GetContainer());
            var w = this.CreateObject() as JSValue;

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
        ~JSContext()
        {
            V8Context_Dispose(context);
        }

        [DllImport(LibName)]
        internal extern static V8Handle V8Context_Create(bool debug, IntPtr logger, IntPtr allocator, IntPtr deAllocator);

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
            [MarshalAs(UnmanagedType.U1)]
            NullableBool configurable,
            [MarshalAs(UnmanagedType.U1)]
            NullableBool enumerable,
            [MarshalAs(UnmanagedType.U1)]
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
        internal extern static V8Response V8Context_ToString(
            V8Handle context,
            IntPtr handle);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_Wrap(
            V8Handle context,
            [MarshalAs(UnmanagedType.LPStruct)]
            object handle);


        [DllImport(LibName)]
        internal extern static void V8Context_Release(V8Response r);


        [DllImport(LibName)]
        internal extern static void V8Context_ReleaseHandle(IntPtr r);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateFunction(
            V8Handle context, IntPtr function, [MarshalAs(UnmanagedType.LPUTF8Str)]string name);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_Evaluate(
            V8Handle context,
            [MarshalAs(UnmanagedType.LPUTF8Str)]
            string script,
            [MarshalAs(UnmanagedType.LPUTF8Str)]
            string location);

    }
}