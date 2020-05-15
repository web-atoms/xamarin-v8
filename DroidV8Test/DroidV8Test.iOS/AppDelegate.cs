using System;
using System.Collections.Generic;
using System.Linq;

using Foundation;
using UIKit;

using Xamarin.Android.V8;

namespace DroidV8Test.iOS
{
    // The UIApplicationDelegate for the application. This class is responsible for launching the 
    // User Interface of the application, as well as listening (and optionally responding) to 
    // application events from iOS.
    [Register("AppDelegate")]
    public partial class AppDelegate : global::Xamarin.Forms.Platform.iOS.FormsApplicationDelegate
    {
        //
        // This method is invoked when the application has loaded and is ready to run. In this 
        // method you should instantiate the window, load the UI into it and then make the window
        // visible.
        //
        // You have 17 seconds to return from this method, or iOS will terminate your application.
        //
        public override bool FinishedLaunching(UIApplication app, NSDictionary options)
        {
            global::Xamarin.Forms.Forms.Init();
            LoadApplication(new App());

            using (var j = new JSContext(true))
            {
                j["a"] = j.CreateString("Akash");
                j["k"] = j.CreateString("Kava");
                var ak = j.Evaluate("`${a} ${k}`");
                // Assert.Equal("Akash Kava", ak.ToString());

                j["n5"] = j.CreateNumber(5);
                var a = j.Evaluate("4 + n5");
                // Assert.True(a.IsNumber);
                // Assert.Equal(a.IntValue, 9);

                j["add"] = j.CreateFunction(0, (c, a1) => {
                    return j.CreateString($"{a1[0]} {a1[1]}");
                }, "Add");

                a = j.Evaluate("add(a, k)");
                // Assert.Equal("Akash Kava", a.ToString());

            }

            return base.FinishedLaunching(app, options);
        }
    }
}
