using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Android.App;
using Android.Content;
using Android.OS;
using Android.Runtime;
using Android.Views;
using Android.Widget;

namespace Xamarin.Android.V8
{
    internal static partial class MainThread
    {

        static volatile Handler handler;

        static bool PlatformIsMainThread
        {
            get
            {
                if (global::Android.OS.Build.VERSION.SdkInt > (BuildVersionCodes.M))
                    return Looper.MainLooper.IsCurrentThread;

                return Looper.MyLooper() == Looper.MainLooper;
            }
        }

        static void PlatformBeginInvokeOnMainThread(Action action)
        {
            if (handler?.Looper != Looper.MainLooper)
                handler = new Handler(Looper.MainLooper);

            handler.Post(action);
        }

        static void PlatformBeginInvokeOnMainThread(Action action, long delayMillis)
        {
            if (handler?.Looper != Looper.MainLooper)
                handler = new Handler(Looper.MainLooper);

            handler.PostDelayed(action, delayMillis);
        }

        public static IDisposable PostTimeout(Action action, long delayMillis)
        {
            if (handler?.Looper != Looper.MainLooper)
                handler = new Handler(Looper.MainLooper);

            return new Timeout(handler, action, delayMillis);
        }


        public static bool IsMainThread =>
            PlatformIsMainThread;

        public static void BeginInvokeOnMainThread(Action action)
        {
            if (IsMainThread)
            {
                action();
            }
            else
            {
                PlatformBeginInvokeOnMainThread(action);
            }
        }

        public static void BeginInvokeOnMainThread(Action action, long delayMillis)
        {
            PlatformBeginInvokeOnMainThread(action, delayMillis);
        }

        public static Task InvokeOnMainThreadAsync(Action action)
        {
            if (IsMainThread)
            {
                action();
#if NETSTANDARD1_0
                return Task.FromResult(true);
#else
                return Task.CompletedTask;
#endif
            }

            var tcs = new TaskCompletionSource<bool>();

            BeginInvokeOnMainThread(() =>
            {
                try
                {
                    action();
                    tcs.TrySetResult(true);
                }
                catch (Exception ex)
                {
                    tcs.TrySetException(ex);
                }
            });

            return tcs.Task;
        }

        public static Task<T> InvokeOnMainThreadAsync<T>(Func<T> func)
        {
            if (IsMainThread)
            {
                return Task.FromResult(func());
            }

            var tcs = new TaskCompletionSource<T>();

            BeginInvokeOnMainThread(() =>
            {
                try
                {
                    var result = func();
                    tcs.TrySetResult(result);
                }
                catch (Exception ex)
                {
                    tcs.TrySetException(ex);
                }
            });

            return tcs.Task;
        }

        public static Task InvokeOnMainThreadAsync(Func<Task> funcTask)
        {
            if (IsMainThread)
            {
                return funcTask();
            }

            var tcs = new TaskCompletionSource<object>();

            BeginInvokeOnMainThread(
                async () =>
                {
                    try
                    {
                        await funcTask().ConfigureAwait(false);
                        tcs.SetResult(null);
                    }
                    catch (Exception e)
                    {
                        tcs.SetException(e);
                    }
                });

            return tcs.Task;
        }

        public static Task<T> InvokeOnMainThreadAsync<T>(Func<Task<T>> funcTask)
        {
            if (IsMainThread)
            {
                return funcTask();
            }

            var tcs = new TaskCompletionSource<T>();

            BeginInvokeOnMainThread(
                async () =>
                {
                    try
                    {
                        var ret = await funcTask().ConfigureAwait(false);
                        tcs.SetResult(ret);
                    }
                    catch (Exception e)
                    {
                        tcs.SetException(e);
                    }
                });

            return tcs.Task;
        }

        public static async Task<SynchronizationContext> GetMainThreadSynchronizationContextAsync()
        {
            SynchronizationContext ret = null;
            await InvokeOnMainThreadAsync(() =>
                ret = SynchronizationContext.Current).ConfigureAwait(false);
            return ret;
        }
    }
}