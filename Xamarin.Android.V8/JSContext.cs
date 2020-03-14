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
    public class JSContext
    {

        SafeHandle context;
        public JSContext()
        {
            this.context = V8Context_Create();
        }

        public JSValue CreateObject()
        {
            return new JSValue(context, V8Context_CreateObject(context).GetContainer());
        }

        public JSValue CreateArray()
        {
            return new JSValue(context, V8Context_CreateArray(context).GetContainer());
        }

        ~JSContext()
        {
            V8Context_Dispose(context);
        }

        [DllImport("libXV8")]
        internal extern static SafeHandle V8Context_Create();

        [DllImport("libXV8")]
        internal extern static void V8Context_Dispose(SafeHandle context);

        [DllImport("libXV8")]
        internal extern static V8Response V8Context_CreateObject(SafeHandle context);


        [DllImport("libXV8")]
        internal extern static V8Response V8Context_CreateNull(SafeHandle context);

        [DllImport("libXV8")]
        internal extern static V8Response V8Context_CreateUndefined(SafeHandle context);

        [DllImport("libXV8")]
        internal extern static V8Response V8Context_CreateArray(SafeHandle context);

        [DllImport("libXV8")]
        internal extern static V8Response V8Context_CreateBoolean(SafeHandle context, bool value);

        [DllImport("libXV8")]
        internal extern static V8Response V8Context_CreateNumber(SafeHandle context, double value);

        [DllImport("libXV8")]
        internal extern static V8Response V8Context_CreateDate(SafeHandle context, long value);

        [DllImport("libXV8")]
        internal extern static V8Response V8Context_CreateString(SafeHandle context, 
            [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

        [DllImport("libXV8")]
        internal extern static V8Response V8Context_GetProperty(
            SafeHandle context, 
            IntPtr handle, 
            [MarshalAs(UnmanagedType.LPUTF8Str)]
            string name);

        [DllImport("libXV8")]
        internal extern static V8Response V8Context_SetProperty(
            SafeHandle context,
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPUTF8Str)]
            string name,
            IntPtr value);

        [DllImport("libXV8")]
        internal extern static V8Response V8Context_GetPropertyAt(SafeHandle context, IntPtr handle, int index);


        [DllImport("libXV8")]
        internal extern static V8Response V8Context_SetPropertyAt(
            SafeHandle context,
            IntPtr handle,
            int index,
            IntPtr value);

        [DllImport("libXV8")]
        internal extern static void V8Context_Release(V8Response r);


        [DllImport("libXV8")]
        internal extern static void V8Context_ReleaseHandle(IntPtr r);
    }
}