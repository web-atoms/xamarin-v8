using System;
using System.Linq;

namespace Xamarin.Android.V8
{
    internal enum V8ResponseType : byte
    {
        Error = 0,
        Handle = 1,
        String = 2,
        Boolean = 3,
        Integer = 4
    }
}