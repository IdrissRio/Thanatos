

/*#############################################################################################
 * This file contains all the methods definition for those function
 * declared within des.h header file.
 *
 * for ANY further modification keep in mind this:
 *  (1): use the type 'byte' defined below instead of 'unsigned char'
 *  (2): use the static constant 'BYTE' variable instead of 'sizeof(unsigned char)' when needed
 *       (declared in 'des.h')
 *
 * Enjoy.
 *
 *############################################################################################*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "des.h"



 void set_padding(byte **message, short *msg_len) {
    if((*msg_len % 8) != 0) {
        
        short new_dim = (*msg_len) + (8-((*msg_len) % 8));
        byte *message_with_padding = malloc(new_dim);
        
        for(short i = 0; i < (*msg_len); ++i)
            message_with_padding[i] = (*message)[i];
        for( short i = (*msg_len); i < new_dim; ++i)
            message_with_padding[i] = '#';
        free(*message);
        *message = message_with_padding;
        message_with_padding = NULL;
        *msg_len = new_dim;
    }
    return;
}

 unsigned char get_bit(byte *in, short pos){
    short pos_byte = pos/8;
    short pos_bit = pos%8;
    byte selected_byte = in[pos_byte];
    return ((selected_byte >> (7-pos_bit)) & 0x1);
}


 void set_bit(byte **in, short pos, short val) {
    
    short pos_byte = pos/8;
    short pos_bit = pos%8;
    byte selected_byte = (*in)[pos_byte];
    selected_byte =  (byte) (((0xFF7F>>pos_bit) & selected_byte) & 0x00FF);
    byte new_byte = (byte)((val << (7-pos_bit)) | selected_byte );
    (*in)[pos_byte] = new_byte;
    
    return;
}

 void select_bits(byte **in, const short *map, short map_len) {
    byte *out = malloc((map_len-1)/8+1);
    for (short i = 0; i < map_len; ++i){
     
        unsigned char val = get_bit(*in, map[i] - 1);
        set_bit(&out, i, val);
    }
    free(*in);
    *in = out;
    out = NULL;
    return;
}

 void select_bits_with_pos(byte **in, short pos, short len) {
    byte *out = malloc((len-1)/8 +1);
    for (short i = pos; i < pos + len; ++i){
        unsigned char val = get_bit(*in, i);
        set_bit(&out, i - pos, val);
    }
    free(*in);
    *in = out;
    out = NULL;
    return;
}


 void select_bits_with_pos_in_key(byte *the_key,
                                 short pos,
                                 short len,
                                 byte **out) {
    if (*out != NULL)
        free(*out);
    *out = malloc((len-1)/8 +1);
    for (short i = pos; i < pos + len; ++i){
        unsigned char val = get_bit(the_key, i);
        set_bit(out, i - pos, val);
    }
    
    return;
}

 void get_subkeys( byte **the_key, byte ***sub_keys, short mode) {
    const short SHIFTS_LEN = 16;
    const short PC1_LEN = 56;
    if (mode == 0)
        // 0 stands for 'encrypt'.
        select_bits(the_key, PC1, PC1_LEN);
    
    byte *c = NULL;
    byte *d = NULL;
    select_bits_with_pos_in_key(*the_key, 0, PC1_LEN/2, &c);
    select_bits_with_pos_in_key(*the_key, PC1_LEN/2, PC1_LEN/2, &d);
    
    *sub_keys = malloc(sizeof(byte *) * SHIFTS_LEN);
    
    for (short i = 0; i < SHIFTS_LEN; ++i){
        
        rotate_left(&c, 28, SHIFTS[i]);
        
        rotate_left(&d, 28, SHIFTS[i]);
        
        
        byte *cd = NULL;
        concatenate_bits(c, 28, d, 28, &cd);
        select_bits(&cd, PC2, 48);
        
        (*sub_keys)[i] = cd;
        
        cd = NULL;
    }
    free(c);
    free(d);
    return;
}



 void concatenate_bits(byte *a, short a_len,
                      byte *b, short b_len,
                      byte **out) {
    if (*out != NULL)
        free(*out);
    *out = malloc((a_len+b_len - 1)/8 + 1);
    int j = 0;
    for (int i = 0; i < a_len; ++i){
        unsigned char val = get_bit(a, i);
        set_bit(out, j, val);
        ++j;
    }
    for (int i = 0; i < b_len; ++i){
        unsigned char val = get_bit(b, i);
        set_bit(out, j, val);
        ++j;
    }
    
    return;
}


 void rotate_left(byte **in, short len, short step) {
    
    byte *copy = malloc(4*BYTE);
    for (short i = 0; i < 4; ++i)
        copy[i] = (*in)[i];
    
    for(short i=0; i<len; ++i) {
        short val = get_bit(copy, (i+step)%len);
        set_bit(in, i, val);
    }
    free(copy);
    
    return;
}


 void split_bytes(byte **in) {
    
    const short in_byte_length = 6;
    const short len = 6;
    
    short num_of_bytes = ((8*in_byte_length-1)/len)+1;
    byte *out = malloc(num_of_bytes * BYTE);
    for (short i = 0; i < num_of_bytes ; ++i)
        out[i] = 0;
    for (short i = 0; i < num_of_bytes; ++i)
        for (short j = 0; j < len; ++j){
            short val = get_bit( *in, len*i+j );
            set_bit( &out, 8*i+j, val );
        }
    free(*in);
    *in = out;
    out = NULL;
    return;
}


 void xor_bytes(byte **a, short a_len,
               byte *b, short b_len) {
    
    byte *copy = malloc(a_len);
    
    for (short i = 0; i < a_len; ++i)
        copy[i] = (*a)[i];
    
    for (short i = 0; i < a_len; ++i )
        (*a)[i] = copy[i] ^ b[i];
    
    free(copy);
    return;
}


 void substitution_6x4(byte **in) {
    
    byte *out = malloc(4);
    split_bytes(in);                                                               // Splitting byte[] into 6-bit blocks
    
    const short in_len = 8;
    
    int lh_byte = 0;
    for (short b = 0; b < in_len; ++b) {                                           // Should be sub-blocks
        byte val_byte = (*in)[b];
        short r = 2*( (val_byte >> 7) & 0x0001) + ( (val_byte >> 2) & 0x0001 );    // 1 and 6
        short c = (val_byte >> 3) & 0x000F;                                        // Middle 4 bits
        short h_byte = S[(64*b)+(16*r)+c];                                         // 4 bits (half byte) output
        
        if (b%2==0) lh_byte = h_byte;                                              // Left half byte
        else out[b/2] = (byte) (16*lh_byte + h_byte);
    }
    free(*in);
    *in = out;
    out = NULL;
    return;
}

 void cipher_encrypt(byte **sub_keys,
                    byte **message,
                    short message_len,
                    byte **modified_message) {
    
    set_padding(message, &message_len);
    if (*modified_message != NULL)
        free(*modified_message);
    *modified_message = malloc(message_len*BYTE);
    
    for (short i = 0; i < message_len;  i += 8) {
        
        byte *partial = malloc(8*BYTE);
        for (short j = 0; j < 8; ++j)
            partial[j] = (*message)[i+j];
        
        select_bits(&partial, IP, 64);
        
        byte *l = NULL;
        byte *r = NULL;
        select_bits_with_pos_in_key(partial, 0, 32, &l);
        select_bits_with_pos_in_key(partial, 32, 32, &r);
        
        
        for (short key_counter = 0 ; key_counter < 16; ++key_counter){
            byte *r_backup = malloc(4*BYTE);
            for (short copy_counter = 0; copy_counter < 4; ++copy_counter)
                r_backup[copy_counter] = r[copy_counter];
            select_bits(&r, E, 48);
            xor_bytes(&r, 6, sub_keys[key_counter], 6);
            
            substitution_6x4(&r);
            select_bits(&r, P, 32);
            xor_bytes(&r, 4, l, 4);
            free(l);
            l = NULL;
            l = r_backup;
            r_backup = NULL;
            
        }
        
        byte *lr = malloc(8*BYTE);
        concatenate_bits(r, 32, l, 32, &lr);
        
        select_bits(&lr, INVP, 64);
        for (short j = 0; j < 8; ++j)
            (*modified_message)[i + j] = lr[j];
        free(lr);
        free(l);
        free(r);
        // be carefl about DP
        free(partial);
        partial = NULL;
    }
    
    return;
}


 void cipher_decrypt(byte **sub_keys,
                    byte **message,
                    byte **modified_message) {
    
    if (*modified_message != NULL)
        free(*modified_message);
    *modified_message = malloc(8*BYTE);
    
    select_bits(message, IP, 64);
    
    byte *l = NULL;
    byte *r = NULL;
    select_bits_with_pos_in_key(*message, 0, 32, &l);
    select_bits_with_pos_in_key(*message, 32, 32, &r);
    
    
    for (short key_counter = 0 ; key_counter < 16; ++key_counter){
        byte *r_backup = malloc(4*BYTE);
        for (short copy_counter = 0; copy_counter < 4; ++copy_counter)
            r_backup[copy_counter] = r[copy_counter];
        select_bits(&r, E, 48);
        xor_bytes(&r, 6, sub_keys[16-key_counter-1], 6);
        
        substitution_6x4(&r);
        select_bits(&r, P, 32);
        xor_bytes(&r, 4, l, 4);
        free(l);
        l = NULL;
        l = r_backup;
        r_backup = NULL;
        
    }
    
    byte *lr = malloc(8*BYTE);
    concatenate_bits(r, 32, l, 32, &lr);
    
    select_bits(&lr, INVP, 64);
    for (short j = 0; j < 8; ++j)
        (*modified_message)[j] = lr[j];
    free(lr);
    free(l);
    free(r);
    // be carefl about DP
    return;
}