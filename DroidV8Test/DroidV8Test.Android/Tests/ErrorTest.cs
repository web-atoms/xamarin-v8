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
using Xamarin.Android.V8;

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
            } catch (JavaScriptException ex)
            {
                Assert.True(ex.Message.StartsWith("SyntaxError: Unexpected end of input"));
            }
        }

        [Test]
        public void NestedError()
        {
            context["throwError"] = context.CreateFunction(0, (c, a) => throw new Exception("Error Thrown"), "Throw");

            try
            {
                context.Evaluate("throwError()");
                Assert.Throw("Expecting an exception");
            } catch (JavaScriptException ex)
            {
                Assert.True(ex.Message.Contains("Error Thrown"));
            }
        }
    }
}