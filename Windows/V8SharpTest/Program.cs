using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WebAtoms;
using WebAtoms.V8Sharp;

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

                    context["add"] = context.CreateFunction(0, (c, a) => c.CreateString($"{a[0]} - {a[1]}") , "Add");

                    name = context.EvaluateTemplate($"add({"Akash"},{"Kava"})");
                    Console.WriteLine(name.ToString());
                }
            }catch(Exception ex) {
                Console.WriteLine(ex);
            }

            Console.ReadLine();

        }
    }
}
