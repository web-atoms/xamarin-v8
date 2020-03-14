using System;
using System.Linq;
using System.Runtime.InteropServices;

namespace Xamarin.Android.V8
{
    public class JSValue
    {
        readonly SafeHandle context;
        private V8HandleContainer handle;
        internal JSValue(SafeHandle context, V8HandleContainer handle)
        {
            this.context = context;
            this.handle = handle;
        }

        public bool IsString => this.handle.handleType == V8HandleType.String;

        public bool IsObject => (handle.handleType & V8HandleType.Object) > 0;

        public bool IsFunction => handle.handleType == V8HandleType.Function;

        public bool IsDate => handle.handleType == V8HandleType.Date;

        public bool IsNumber => handle.handleType == V8HandleType.Number || handle.handleType == V8HandleType.NotANumber;

        public bool IsBoolean => handle.handleType == V8HandleType.Boolean;

        public bool IsArray => handle.handleType == V8HandleType.Array;

        public JSValue this[string name]
        {
            get
            {
                return new JSValue(context, JSContext.V8Context_GetProperty(context, handle.handle, name).GetContainer());
            }
            set
            {
                var v = value ?? (new JSValue(context, JSContext.V8Context_CreateNull(context).GetContainer()));
                JSContext.V8Context_SetProperty(context, handle.handle, name, v.handle.handle).GetContainer();
            }
        }

        public JSValue this[int index]
        {
            get
            {
                return new JSValue(context, JSContext.V8Context_GetPropertyAt(context, handle.handle, index).GetContainer());
            }
            set
            {
                var v = value ?? (new JSValue(context, JSContext.V8Context_CreateNull(context).GetContainer()));
                JSContext.V8Context_SetPropertyAt(context, handle.handle, index, v.handle.handle).GetContainer();
            }
        }


        ~JSValue()
        {
            if (handle.handle != IntPtr.Zero)
            {
                JSContext.V8Context_ReleaseHandle(handle.handle);
            }
        }
    }
}