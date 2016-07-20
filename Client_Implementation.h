//
//  Client_Implementation.h
//  Server
//
//  Created by IdrissRio on 28/04/16.
//  Copyright © 2016 RIFT. All rights reserved.
//

#ifndef Client_Implementation_h
#define Client_Implementation_h

#include <stdio.h>
void testOffset(unsigned long long int START_OFFSET, unsigned long long int END_OFFSET, byte **MODIFIED_MESSAGE, byte *success, int processID);


void testOffsetHybridCpu(byte **MODIFIED_MESSAGE, byte *SUCCESS,int processID);
#endif /* Client_Implementation_h */
