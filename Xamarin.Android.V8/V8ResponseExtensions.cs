using System;
using System.Linq;
using System.Runtime.InteropServices;

namespace Xamarin.Android.V8
{
    internal static class V8ResponseExtensions
    {

        internal static long EPOCH = (new DateTime(1970, 1, 1)).Ticks;

        internal static Int64 ToJSTime(this DateTime dt)
        {
            return (Int64)((dt.Ticks - EPOCH) / 1000);
        }

        internal static DateTime FromJSTime(this long value)
        {
            var t = (value * 1000) + EPOCH;
            return new DateTime(t, DateTimeKind.Utc);
        }

        internal static string ToUTF8StringFromPtr(this IntPtr utf8Ptr)
        {
            if (utf8Ptr == IntPtr.Zero)
            {
                return null;
            }
            string s = Marshal.PtrToStringUTF8(utf8Ptr);
            Marshal.FreeHGlobal(utf8Ptr);
            return s;
        }

        internal static void ThrowError(V8Response r)
        {
            if (r.type == V8ResponseType.Error)
            {
                string msg = r.error.message.ToUTF8StringFromPtr();
                if (r.error.stack != IntPtr.Zero)
                {
                    msg += "\r\n" + r.error.stack.ToUTF8StringFromPtr();
                }
                JSContext.V8Context_Release(r);
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