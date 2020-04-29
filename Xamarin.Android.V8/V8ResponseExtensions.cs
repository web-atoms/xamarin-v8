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

        internal unsafe static void ThrowError(V8Response r)
        {
            if (r.type == V8ResponseType.Error || r.type == V8ResponseType.ConstError)
            {
                var msg = r.result.array.stringValue;
                var str = new String((char*)msg, 0, r.result.array.Length);
                if (r.type == V8ResponseType.Error)
                {
                    Marshal.FreeHGlobal(msg);
                }
                throw new Exception(str);
            }
        }

        internal static V8HandleContainer GetContainer(this V8Response r)
        {
            ThrowError(r);
            if (r.type != V8ResponseType.Handle)
            {
                // JSContext.V8Context_Release(r);
                throw new NotSupportedException();
            }
            return r.result.handle;
        }

        internal static bool GetBooleanValue(this V8Response r)
        {
            ThrowError(r);
            if (r.type != V8ResponseType.Boolean)
            {
                // JSContext.V8Context_Release(r);
                throw new NotSupportedException();
            }
            return r.result.booleanValue;
        }

        internal static int GetIntegerValue(this V8Response r)
        {
            ThrowError(r);
            if (r.type != V8ResponseType.Integer)
            {
                // JSContext.V8Context_Release(r);
                throw new NotSupportedException();
            }
            return r.result.intValue;
        }


        internal unsafe static string GetString(this V8Response r)
        {
            ThrowError(r);
            if (!(r.type == V8ResponseType.String || r.type == V8ResponseType.ConstString))
            {
                // JSContext.V8Context_Release(r);
                throw new NotSupportedException();
            }
            var value = r.result.array.stringValue;
            var result = r.result.array.String;
            if (r.type == V8ResponseType.String)
            {
                Marshal.FreeHGlobal(value);
            }
            return result;
        }
    }
}