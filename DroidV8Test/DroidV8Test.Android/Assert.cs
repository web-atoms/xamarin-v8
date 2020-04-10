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

namespace DroidV8Test.Droid
{
    public class Assert
    {
        public static void True(bool result)
        {
            Equal(true, result);
        }

        public static void Null(object checkNull)
        {
            Equal(null, checkNull);
        }

        public static void NotNull(object checkNull)
        {
            if (checkNull == null)
                throw new Exception("Expected null and Found not null");
        }

        public static void Equal<T>(T expected, T result)
            where T: IEquatable<T>
        {
            if (expected == null)
            {
                if (result == null)
                {
                    return;
                }
                Throw($"Expected null and found {result}");
            }
            if (result == null)
            {
                Throw($"Expected {expected} and found null");
            }
            if (!expected.Equals(result))
                Throw($"Expected {expected} and found {result}");
        }

        public static void Equal(object expected, object result)
        {
            if (expected == null)
            {
                if (result == null)
                {
                    return;
                }
                Throw($"Expected null and found {result}");
            }
            if (result == null)
            {
                Throw($"Expected {expected} and found null");
            }
            if (!expected.Equals(result))
                Throw($"Expected {expected} and found {result}");
        }

        public static void Throw(string error)
        {
            throw new Exception(error);
        }
    }
}