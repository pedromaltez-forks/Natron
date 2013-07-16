//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com
#ifndef OPERATOR_H
#define OPERATOR_H
#include "Core/node.h"
class Op : public Node
{
public:

    Op();
    
    virtual bool cacheData()=0;
    
    virtual ~Op(){}


};

#endif // OPERATOR_H
