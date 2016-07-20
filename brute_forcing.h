//
//  brute_forcing.h
//  Distributed_des
//
//  Created by Tommaso Campari on 22/04/16.
//  Copyright © 2016 RIFT. All rights reserved.
//

#ifndef brute_forcing_h
#define brute_forcing_h

#include "stdio.h"
#include "stdlib.h"

#include "des.h"


short get_bit_from_longlong (long long in, short pos);
void generate_key_from_longlong_value (long long key_value, byte **out, short mode);
short compare_strings_with_8_elements (byte *a, byte *b);
short try_decrypt (long long key, byte *cryptedMsg);
void full_decrypt(long long key, byte *THE_MESSAGE, const int MESSAGE_SIZE);

#endif /* brute_forcing_h */
