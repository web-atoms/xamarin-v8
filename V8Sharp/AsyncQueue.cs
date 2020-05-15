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
    internal class AsyncQueue<T>
    {
        Queue<T> queue = new Queue<T>();
        TaskCompletionSource<T> locker;

        public T UnUsedMessage
        {
            get
            {
                lock (queue) {
                    if (locker == null)
                    {
                        if(queue.Count > 1)
                        {
                            return queue.Dequeue();
                        }
                    }
                }
                return default;
            }
        }

        public void Enqueue(T item)
        {
            lock (queue)
            {
                queue.Enqueue(item);
                if (locker != null)
                {
                    locker.TrySetResult(item);
                }
            }
        }

        public Task<T> DequeueAsync()
        {
            lock(queue)
            {
                T item = default;
                if (queue.TryDequeue(out item))
                {
                    locker = null;
                    return Task.FromResult(item);
                }
                locker = new TaskCompletionSource<T>();
            }
            return locker.Task;
        }
    }
}