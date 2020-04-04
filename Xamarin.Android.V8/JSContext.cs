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


namespace Xamarin.Android.V8
{

    internal delegate V8Response ExternalCall(V8Response thisHandle, V8Response args);

    public delegate JSValue Function(JSValue jsThis, JSValue jsArgs);

    internal delegate IntPtr MemoryAllocator(int len);

    public class JSContext
    {

        const string LibName = "v8android";

        static IntPtr allocator;
        static IntPtr deAllocator;

        SafeHandle context;
        public JSContext(bool debug = false)
        {
            if (allocator == null)
            {
                MemoryAllocator a = (n) => Marshal.AllocHGlobal(n);
                allocator = Marshal.GetFunctionPointerForDelegate(a);
                deAllocator = IntPtr.Zero;
            }
            this.context = V8Context_Create(debug, allocator, deAllocator);
        }

        public JSValue CreateObject()
        {
            return new JSValue(context, V8Context_CreateObject(context).GetContainer());
        }

        public JSValue CreateArray()
        {
            return new JSValue(context, V8Context_CreateArray(context).GetContainer());
        }

        public JSValue CreateString(string value)
        {
            return new JSValue(context, V8Context_CreateString(context, value).GetContainer());
        }

        public JSValue CreateNumber(double value)
        {
            return new JSValue(context, V8Context_CreateNumber(context, value).GetContainer());
        }

        public JSValue CreateDate(DateTime value)
        {
            return new JSValue(context, V8Context_CreateDate(context, value.ToJSTime()).GetContainer());
        }

        public JSValue CreateFunction(Function fx, string debugDescription)
        {
            ExternalCall efx = (t, a) => {
                var tjs = new JSValue(context, t.GetContainer());
                var targs = new JSValue(context, t.GetContainer());
                try
                {
                    var r = fx(tjs, targs);
                    return new V8Response {
                        type = V8ResponseType.Handle,
                        handle = new V8HandleContainer {
                            handle = r.Detach()
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
            return new JSValue(context, c);
        }

        public JSValue Evaluate(string script, string location = null)
        {
            var c = V8Context_Evaluate(context, script, location ?? "Unnamed").GetContainer();
            return new JSValue(context, c);
        }

        ~JSContext()
        {
            V8Context_Dispose(context);
        }

        [DllImport(LibName)]
        internal extern static SafeHandle V8Context_Create(bool debug, IntPtr allocator, IntPtr deAllocator);

        [DllImport(LibName)]
        internal extern static void V8Context_Dispose(SafeHandle context);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateObject(SafeHandle context);


        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateNull(SafeHandle context);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateUndefined(SafeHandle context);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateArray(SafeHandle context);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateBoolean(SafeHandle context, bool value);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateNumber(SafeHandle context, double value);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateDate(SafeHandle context, long value);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateString(SafeHandle context, 
            [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_GetProperty(
            SafeHandle context, 
            IntPtr handle, 
            [MarshalAs(UnmanagedType.LPUTF8Str)]
            string name);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_SetProperty(
            SafeHandle context,
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPUTF8Str)]
            string name,
            IntPtr value);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_GetPropertyAt(SafeHandle context, IntPtr handle, int index);


        [DllImport(LibName)]
        internal extern static V8Response V8Context_SetPropertyAt(
            SafeHandle context,
            IntPtr handle,
            int index,
            IntPtr value);

        [DllImport(LibName)]
        internal extern static void V8Context_Release(V8Response r);


        [DllImport(LibName)]
        internal extern static void V8Context_ReleaseHandle(IntPtr r);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_CreateFunction(
            SafeHandle context, IntPtr function, [MarshalAs(UnmanagedType.LPUTF8Str)]string name);

        [DllImport(LibName)]
        internal extern static V8Response V8Context_Evaluate(
            SafeHandle context,
            [MarshalAs(UnmanagedType.LPUTF8Str)]
            string script,
            [MarshalAs(UnmanagedType.LPUTF8Str)]
            string location);
    }
}