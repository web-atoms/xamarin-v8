using System;
using System.Linq;

namespace Xamarin.Android.V8
{
    internal enum V8HandleType : byte
    {
        None = 0,
        Undefined = 1,
        Null = 2,
        Boolean = 3,
        Number = 8,
        NotANumber = 9,
        Integer = 10,
        BigInt = 11,
        String = 0xFF,
        Object = 0xF0,
        Function = 0xF1,
        Array = 0xF2,
        Date = 0xF3,
        Wrapped = 0xF4,
        WrappedFunction = 0xF5,
        Symbol = 0xF6
    }
}