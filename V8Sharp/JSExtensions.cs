using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using WebAtoms;
using V8Handle = System.IntPtr;

namespace Xamarin.Android.V8
{

    public class JavaScriptException: Exception
    {
        public JavaScriptException(string message): base(message)
        {

        }
    }

    internal static class JSExtensions
    {
        public static JSValue ToJSValue(this IJSValue v)
        {
            return ((JSValue)v);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static V8Handle ToHandle(this IJSValue v, JSContext context)
        {
            if (v == null)
            {
                return context.Undefined.handle.address;
            }
            return ((JSValue)v).handle.address;
        }


        private static V8Handle[] EmptyHandles = new V8Handle[0];

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static V8Handle[] ToHandles(this IJSValue[] v, JSContext context)
        {
            if (v == null || v.Length == 0)
                return EmptyHandles;
            var a = new V8Handle[v.Length];
            for (int i = 0; i < v.Length; i++)
            {
                var vi = v[i];
                a[i] = vi == null ? context.Undefined.handle.address : ((JSValue)vi).handle.address;
            }
            return a;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static V8Handle[] ToHandles(this IList<IJSValue> v, JSContext context)
        {
            if (v == null || v.Count == 0)
                return EmptyHandles;
            var len = v.Count;
            var a = new V8Handle[len];
            int i = 0;
            foreach (var vi in v)
            {
                a[i++] = vi == null ? context.Undefined.handle.address : ((JSValue)vi).handle.address;
            }
            return a;
        }

    }
}