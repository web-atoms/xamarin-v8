using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WebAtoms;
using Xamarin.Android.V8;

namespace V8SharpTest
{
    class Program
    {
        static void Main(string[] args)
        {

            try
            {
                using (JSContext context = new JSContext(false))
                {
                    var name = context.EvaluateTemplate($"`${{{"Akash"}}} ${{{"Kava"}}}`");
                    Console.WriteLine(name.ToString());
                }
            }catch(Exception ex) {
                Console.WriteLine(ex);
            }

            Console.ReadLine();

        }
    }
}
