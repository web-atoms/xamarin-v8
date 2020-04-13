using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using Android.App;
using Android.Content;
using Android.OS;
using Android.Runtime;
using Android.Views;
using Android.Widget;

namespace DroidV8Test.Droid.Tests
{
    public class ErrorTest: BaseTest
    {

        [Test]
        public void CompileError()
        {
            try
            {
                context.Evaluate("13```sdfdsfd");
                Assert.Throw("Expecting an exception");
            } catch (Exception  ex)
            {
                Assert.True(ex.Message.StartsWith("SyntaxError: Unterminated template literal"));
            }
        }

    }
}