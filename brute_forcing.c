//
//  brute_forcing.c
//  Distributed_des
//
//  Created by Tommaso Campari on 22/04/16.
//  Copyright © 2016 RIFT. All rights reserved.
//
#include "brute_forcing.h"
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

extern int verbose_set;


 short get_bit_from_longlong (long long in, short pos){
    return ((in >> (63-pos)) & 0x1);
}

 short compare_strings_with_8_elements (byte *a, byte *b){
    for (short i = 0; i < 8; ++i)
        if (a[i] != b[i])
            return 0;
    return 1;
}

 void generate_key_from_longlong_value (long long key_value, byte **out, short mode){
    if (*out != NULL)
        free(*out);
    if (mode ==1){
        *out = malloc(7 * BYTE);
        for (short i = 8; i < 64; ++i){
            short val = get_bit_from_longlong(key_value, i);
            set_bit(out, i - 8, val);
        }
    }
    else{
        *out = malloc(8 * BYTE);
        for (short i = 0; i < 64; ++i){
            short val = get_bit_from_longlong(key_value, i);
            set_bit(out, i, val);
        }
    }
}

 short try_decrypt (long long key, byte *cryptedMsg){
    
    byte *partial = malloc(8*BYTE);
    for (short i = 0; i < 8; ++i){
        partial[i] = cryptedMsg[i];
    }
    byte *the_key = malloc(7*BYTE);
    byte **sub_keys = NULL;
    generate_key_from_longlong_value(key, &the_key, 1);
    get_subkeys(&the_key, &sub_keys, 1);
    byte *out = NULL;
    cipher_decrypt(sub_keys, &partial, &out);
    if (compare_strings_with_8_elements(out, (unsigned char *)"il messa") == 1){
        for (short i = 0; i < 8; ++i)
            partial[i] = cryptedMsg[8 + i];
        cipher_decrypt(sub_keys, &partial, &out);
        
        if (compare_strings_with_8_elements(out, (unsigned char *)"ggio seg") == 1){
            for (short i = 0; i < 8; ++i)
                partial[i] = cryptedMsg[16 + i];
            cipher_decrypt(sub_keys, &partial, &out);
            
            if (compare_strings_with_8_elements(out, (unsigned char *)"reto e':") == 1){
                free(partial);
                free(the_key);
                for (short i = 0; i < 16; ++i )
                    free(sub_keys[i]);
                free(out);
                return 1;
            }
            
        }
        
    }
    
    free(partial);
    free(the_key);
    for (short i = 0; i < 16; ++i )
        free(sub_keys[i]);
    free(sub_keys);
    free(out);
     
    
    
    return 0;
}


 void full_decrypt(long long key, byte *THE_MESSAGE, const int MESSAGE_SIZE) {
    if(verbose_set){
    printf(BLU"\n\n######## DECRIPTING IN CORSO ######## \n"RESET);
    }
    byte *decrypted_message = malloc(BYTE*MESSAGE_SIZE);   //Alloco la memoria che conterrà il messaggio decriptato
    byte *partial = malloc(BYTE);                          // Alloco la memoria per che verrà utilizzata in chper_Decript
    byte *the_key = malloc(7*BYTE);
    byte **sub_keys = NULL;
    generate_key_from_longlong_value(key, &the_key, 1);   //Genera la chiave di cifratura/decifratura a partire da un long long
    get_subkeys(&the_key, &sub_keys, 1);                  //Prente le 16 sotto chiavi
    byte *out = NULL;
    for(int i = 0; i < MESSAGE_SIZE; i += 8) {
        for(int j = 0; j < 8; ++j)
            partial[j] = THE_MESSAGE[i+j];
        cipher_decrypt(sub_keys, &partial, &out);
        for (int j = 0; j < 8; ++j){
            decrypted_message[i+j] = out[j];
        }
    }
    for(int i = 0; i <16 ;++i)
        free(sub_keys[i]);
    free(sub_keys);
    printf(RED"\nLa chiave trovata e': "WHT"%llu"RED".", key);
    if(verbose_set){
        printf(RED"\n\n########  Testo cifrato ##########\n"MAG);
        for(int i=0;i<MESSAGE_SIZE;i++){
            printf("%c",decrypted_message[i]);
        }
        printf(RED"\n########  Testo cifrato ##########\n"MAG);
        printf("\n"RESET);
        printf(BLU"\n\n######## FINE DECRIPTING  ######## "RESET);
    }
    fflush(stdout);
    free(out);
    free(partial);
    free(the_key);
    free(decrypted_message);
}
