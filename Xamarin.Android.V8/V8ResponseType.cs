using System;
using System.Linq;

namespace Xamarin.Android.V8
{
    internal enum V8ResponseType : short
    {
        Error = 0,
        Handle = 1,
        String = 2
    }
}