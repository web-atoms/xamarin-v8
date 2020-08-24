using System;
using System.Linq;

namespace WebAtoms.V8Sharp
{
    internal enum V8HandleType : int
    {
        None = 0,
        Undefined = 1,
        Null = 2,
        Boolean = 3,
        Number = 8,
        NotANumber = 9,
        Integer = 10,
        BigInt = 11,
        String = 0xFF,
        Object = 0xF0,
        Function = 0xF1,
        Array = 0xF2,
        Date = 0xF3,
        Wrapped = 0xF4,
        WrappedFunction = 0xF5,
        TypeSymbol = 0xF6,

        // these are array pointers...
        // intValue contains length

        CharArray = 0x12,
        ConstCharArray = 0x13,
        Error = 0x14,
        ConstError = 0x15,

        ResponseArray = 0x16
    }
}