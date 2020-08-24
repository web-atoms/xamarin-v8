using System;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace WebAtoms.V8Sharp
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

        internal unsafe static string ToUtf16String(this IntPtr value, int len = 0)
        {
            if (len == 0)
            {
                return new String((char*)value);
            }
            return new String((char*)value, 0, len);
        }

        internal static void ThrowError(this V8Response r)
        {
            if (r.Type == V8HandleType.Error || r.Type == V8HandleType.ConstError)
            {
                var msg = r.StringValue;
                throw new JavaScriptException(msg);
            }
        }

        internal static bool GetBooleanValue(this V8Response r)
        {
            ThrowError(r);
            if (r.Type != V8HandleType.Boolean)
            {
                // JSContext.V8Context_Release(r);
                throw new NotSupportedException();
            }
            return r.result.booleanValue;
        }

        internal static int GetIntegerValue(this V8Response r)
        {
            ThrowError(r);
            if (r.Type != V8HandleType.Integer)
            {
                // JSContext.V8Context_Release(r);
                throw new NotSupportedException();
            }
            return r.result.intValue;
        }

    }
}