using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Android.App;
using Android.Content;
using Android.OS;
using Android.Runtime;
using Android.Views;
using Android.Widget;

namespace Xamarin.Android.V8
{
    internal class AtomAsyncDispatcher : IDisposable
    {
        public static AtomAsyncDispatcher Instance = new AtomAsyncDispatcher();

        private bool isRunning = false;

        private Queue<Func<Task>> tasks = new Queue<Func<Task>>();

        public void EnqueueTask(Func<Task> task)
        {
            lock (tasks)
            {
                tasks.Enqueue(task);
            }
            var app = global::Android.App.Application.Context.ApplicationContext as global::Android.App.Application;
            
            Task.Run(() => this.DequeueTaskAsync());
        }

        async Task DequeueTaskAsync()
        {
            if (isRunning)
                return;
            isRunning = true;
            try
            {
                Func<Task> task = null;
                lock (tasks)
                {
                    if (!tasks.TryDequeue(out task))
                    {
                        return;
                    }
                }

                await task();
            }
            finally
            {
                isRunning = false;
            }

            await this.DequeueTaskAsync();
        }

        public void Dispose()
        {

        }
    }
}