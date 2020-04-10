using System;
using Xamarin.UITest;
using Xamarin.UITest.Queries;

namespace Xamarin.Android.V8.Tests
{
    public class AppInitializer
    {
        public static IApp StartApp(Platform platform)
        {
            if (platform == Platform.Android)
            {
                return ConfigureApp.Android.StartApp();
            }

            return ConfigureApp.iOS.StartApp();
        }
    }
}