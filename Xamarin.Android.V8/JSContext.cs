using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

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

    internal delegate V8Response ExternalCall(V8Response thisHandle, V8Response args);

    public delegate JSValue Function(JSValue jsThis, JSValue jsArgs);

    internal delegate IntPtr MemoryAllocator(int len);

    internal delegate void JSContextLog(IntPtr text);

    public class JSContext: IJSContext
    {

        const string LibName = "liquidjs";

        static IntPtr allocator;
        static IntPtr deAllocator;
        static IntPtr logger;

        public Action<string> Logger { get; set; }

        internal V8Handle context;

        public event EventHandler<ErrorEventArgs> ErrorEvent;

        public IJSValue Undefined { get; }

        public IJSValue Global { get; }

        public string Stack => throw new NotImplementedException();

        public IJSValue this[string name] { get => throw new NotImplementedException(); set => throw new NotImplementedException(); }

        public JSContext(bool debug = false)
        {
            if (allocator == IntPtr.Zero)
            {
                MemoryAllocator a = (n) => {
                    var an = Marshal.AllocHGlobal(n);
                    return an;
                };
                JSContextLog _logger = (t) => {
                    var s = Marshal.PtrToStringUTF8(t);
                    Logger?.Invoke(s);

                    // to do free string...
                };
                allocator = Marshal.GetFunctionPointerForDelegate(a);
                logger = Marshal.GetFunctionPointerForDelegate(_logger);
                deAllocator = IntPtr.Zero;
            }
            this.context = V8Context_Create(debug, logger, allocator, deAllocator);

            if (Logger == null)
            {
                Logger = (s) => {
                    System.Diagnostics.Debug.WriteLine(s);
                };
            }

            this.Undefined = new JSValue(this, V8Context_CreateUndefined(context).GetContainer());

            this.Global = new JSValue(this, V8Context_GetGlobal(context).GetContainer());
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

        public IJSValue CreateNumber(double value)
        {
            return new JSValue(this, V8Context_CreateNumber(context, value).GetContainer());
        }

        public IJSValue CreateDate(DateTime value)
        {
            return new JSValue(this, V8Context_CreateDate(context, value.ToJSTime()).GetContainer());
        }

        public IJSValue CreateFunction(int args, Func<IJSContext, IJSValue[], IJSValue> fx, string debugDescription)
        {
            ExternalCall efx = (t, a) => {
                var tjs = new JSValue(this, t.GetContainer());
                var targs = new JSValue(this, t.GetContainer());
                var len = targs.Length;
                var al = new IJSValue[len];
                for (int i = 0; i < len; i++)
                {
                    al[i] = targs[i];
                }
                try
                {
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

            var c = V8Context_CreateFunction(context, Marshal.GetFunctionPointerForDelegate(efx), debugDescription).GetContainer();
            return new JSValue(this, c);
        }

        public IJSValue Evaluate(string script, string location = null)
        {
            var c = V8Context_Evaluate(context, script, location ?? "vm" ).GetContainer();
            return new JSValue(this, c);
        }

        public IJSValue Convert(object value)
        {
            throw new NotImplementedException();
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
        internal extern static V8Response V8Context_DeleteProperty(
            V8Handle context,
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPUTF8Str)]
            string name);

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