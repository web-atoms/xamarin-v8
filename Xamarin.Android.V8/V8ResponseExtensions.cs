using System;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace Xamarin.Android.V8
{
    internal static class V8ResponseExtensions
    {

        internal static NullableBool ToNullableBool(this bool? value)
        {
            if (value == null)
            {
                return NullableBool.NotSet;
            }
            if (value.Value == true)
                return NullableBool.True;
            return NullableBool.False;
        }


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

        internal static string ToUTF8StringFromPtr(this IntPtr utf8Ptr, bool delete = false)
        {
            if (utf8Ptr == IntPtr.Zero)
            {
                return null;
            }
            string s = Marshal.PtrToStringUTF8(utf8Ptr);
            if (delete)
            {
                Marshal.FreeHGlobal(utf8Ptr);
            }
            return s;
        }

        internal static void ThrowError(V8Response r)
        {
            if (r.type == V8ResponseType.Error)
            {
                string msg = r.result.error.message.ToUTF8StringFromPtr();
                if (r.result.error.stack != IntPtr.Zero)
                {
                    msg += "\r\n" + r.result.error.stack.ToUTF8StringFromPtr();
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
            return r.result.handle;
        }

        internal static bool GetBooleanValue(this V8Response r)
        {
            ThrowError(r);
            if (r.type != V8ResponseType.Boolean)
            {
                JSContext.V8Context_Release(r);
                throw new NotSupportedException();
            }
            return r.result.booleanValue;
        }

        internal static int GetIntegerValue(this V8Response r)
        {
            ThrowError(r);
            if (r.type != V8ResponseType.Integer)
            {
                JSContext.V8Context_Release(r);
                throw new NotSupportedException();
            }
            return r.result.intValue;
        }


        internal static string GetString(this V8Response r)
        {
            ThrowError(r);
            if (r.type != V8ResponseType.String)
            {
                JSContext.V8Context_Release(r);
                throw new NotSupportedException();
            }
            return r.result.stringValue.ToUTF8StringFromPtr(true);
        }
    }
}