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
using WebAtoms;

namespace DroidV8Test.Droid.Tests
{
    public class FunctionTest: BaseTest
    {

        [Test]
        public void Function()
        {
            context["clrFunction"] = context.CreateFunction(0, (c, a) => c.Convert("Test"), "T1");
            var a = context.Evaluate("clrFunction()");
            Assert.True(a.IsString);
            Assert.Equal("Test", a.ToString());
        }

        [Test]
        public void FunctionWithParameters()
        {
            context["clrFunction"] = context.CreateFunction(0, (c, a) => c.Convert($"{a[0]} {a[1]}") , "T2");
            var a = context.Evaluate("clrFunction('Akash', 'Kava')");
            Assert.Equal("Akash Kava", a.ToString());

            context.Evaluate("function add(f, l) { return `${f} ${l}`; }");
            var c = context["add"];
            a = c.InvokeFunction(null, context.CreateString("Simmi"), context.CreateString("Kava"));
            Assert.Equal("Simmi Kava", a.ToString());
        }

        [Test]
        public void InvokeTest()
        {
            var math = context.CreateObject();
            context["Math"] = math;
            math["add"] = context.CreateFunction(1, (c, a) => {
                var p0 = a[0].DoubleValue;
                var p1 = a[1].DoubleValue;

                return context.CreateNumber(p0 + p1);
            }, "Add Method");

            var r = context.Evaluate("Math.add(4, 4)");
            Assert.Equal(8, r.IntValue);

            r = math.InvokeMethod("add", context.CreateNumber(4), context.CreateNumber(5));
            Assert.Equal(9, r.IntValue);
        }

        [Test]
        public void SerializeTest()
        {
            var m = new Math();
            var mv = context.Serialize(m, SerializationMode.Reference);
            context["math"] = mv;
            var r = context.Evaluate("math.add(4, 5)");
            Assert.Equal(9, r.IntValue);
            Assert.Equal(m, context.Deserialize<Math>(mv));
        }

    }

    public class Math
    {
        public int Add(int a, int b) => a + b;
    }
}