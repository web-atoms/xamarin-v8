//
//  xv8.cpp
//  xv8
//
//  Created by user169533 on 5/7/20.
//  Copyright © 2020 NeuroSpeech. All rights reserved.
//

#include <iostream>
#include "xv8.hpp"
#include "xv8Priv.hpp"

void xv8::HelloWorld(const char * s)
{
    xv8Priv *theObj = new xv8Priv;
    theObj->HelloWorldPriv(s);
    delete theObj;
};

void xv8Priv::HelloWorldPriv(const char * s) 
{
    std::cout << s << std::endl;
};

