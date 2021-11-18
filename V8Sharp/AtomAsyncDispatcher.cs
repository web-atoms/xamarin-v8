using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
//using Android.App;
//using Android.Content;
//using Android.OS;
//using Android.Runtime;
//using Android.Views;
//using Android.Widget;

namespace Xamarin.Android.V8
{
    internal class AtomAsyncDispatcher : IDisposable
    {
        public static AtomAsyncDispatcher Instance = new AtomAsyncDispatcher();

        private Task previous = null;

        public AtomAsyncDispatcher()
        {
        }


        public void EnqueueTask(Func<Task> task)
        {
            lock (this)
            {
                if(previous == null)
                {
                    previous = Task.Run(task);
                    return;
                }
                previous = Task.Run(async () => {
                    await previous;
                    await task();
                });
            }
        }

        public void Dispose()
        {
            
        }
    }
}