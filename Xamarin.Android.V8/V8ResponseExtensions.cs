using System;
using System.Linq;
using System.Runtime.InteropServices;

namespace Xamarin.Android.V8
{
    internal static class V8ResponseExtensions
    {
        internal static void ThrowError(V8Response r)
        {
            if (r.type == V8ResponseType.Error)
            {
                string stack = null;
                string msg = null;
                msg = Marshal.PtrToStringUTF8(r.error.message);
                if (r.error.stack != IntPtr.Zero)
                {
                    stack = Marshal.PtrToStringUTF8(r.error.stack);
                }
                JSContext.V8Context_Release(r);
                if (stack != null)
                {
                    msg = msg + "\r\n" + stack;
                }
                throw new Exception(msg);
            }
        }

        internal static V8HandleContainer GetContainer(this V8Response r)
        {
            ThrowError(r);
            if (r.type != V8ResponseType.Handle)
            {
                JSContext.V8Context_Release(r);
                throw new NotSupportedException();
            }
            return r.handle;
        }

        internal static string GetString(this V8Response r)
        {
            ThrowError(r);
            if (r.type != V8ResponseType.String)
            {
                JSContext.V8Context_Release(r);
                throw new NotSupportedException();
            }
            var msg = Marshal.PtrToStringUTF8(r.stringValue);
            JSContext.V8Context_Release(r);
            return msg;
        }
    }
}