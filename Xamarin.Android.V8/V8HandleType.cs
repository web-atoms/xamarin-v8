using System;
using System.Linq;

namespace Xamarin.Android.V8
{
    internal enum V8HandleType : short
    {
        None = 0,
        Undefined = 1,
        Null = 2,
        Number = 3,
        NotANumber = 4,
        BigInt = 5,
        Boolean = 6,
        String = 0xFF,
        Object = 0xF0,
        Function = 0xF1,
        Array = 0xF2,
        Remote = 0xF3,
        Date = 0xF4
    }
}