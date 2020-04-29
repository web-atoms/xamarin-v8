using System;
using System.Linq;

namespace Xamarin.Android.V8
{
    internal enum V8ResponseType : byte
    {
        Error = 0,
        ConstError = 1,
        Handle = 2,
        String = 3,
        ConstString = 4,
        Boolean = 5,
        Integer = 6,
        Array = 7
    }
}