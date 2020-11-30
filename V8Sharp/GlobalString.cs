using System;
using System.Collections.Generic;
using System.Text;

namespace Xamarin.Android.V8
{
    public struct GlobalString
    {

        public IntPtr Pointer;

        public string Value;

        public static implicit operator IntPtr(GlobalString @string)
        {
            return @string.Pointer;
        }

    }
}
