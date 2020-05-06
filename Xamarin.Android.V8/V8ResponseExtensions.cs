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

        internal static void ThrowError(this V8Response r)
        {
            if (r.Type == V8HandleType.Error || r.Type == V8HandleType.ConstError)
            {
                var msg = r.StringValue;
                if (r.Type == V8HandleType.Error)
                {
                    Marshal.FreeHGlobal(r.address);
                }
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