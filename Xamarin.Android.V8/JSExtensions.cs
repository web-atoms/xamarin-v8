using System;
using System.Linq;
using WebAtoms;
using V8Handle = System.IntPtr;

namespace Xamarin.Android.V8
{
    internal static class JSExtensions
    {
        public static JSValue ToJSValue(this IJSValue v)
        {
            return ((JSValue)v);
        }

        public static V8Handle ToHandle(this IJSValue v)
        {
            if (v == null)
            {
                return IntPtr.Zero;
            }
            return ((JSValue)v).handle.handle;
        }


        private static V8Handle[] EmptyHandles = new V8Handle[0];

        public static V8Handle[] ToHandles(this IJSValue[] v)
        {
            if (v == null || v.Length == 0)
                return EmptyHandles;
            var a = new V8Handle[v.Length];
            for (int i = 0; i < v.Length; i++)
            {
                a[i] = ((JSValue)v[i]).handle.handle;
            }
            return a;
        }

    }
}