 //
//  Client_ImplementationGPU.c
//  Server
//
//  Created by IdrissRio on 26/05/16.
//  Copyright © 2016 RIFT. All rights reserved.
//

#include "Client_ImplementationGPU.cuh"
#include "mpi.h"
#include "omp.h"




__device__ char mess_known[] = {'i', 'l', ' ', 'm', 'e', 's', 's', 'a', 'g', 'g', 'i', 'o', ' ', 's', 'e', 'g', 'r', 'e', 't', 'o', ' ', 'e', 39, ':', 0};


__host__ __device__ unsigned char get_bit_long_cu(byte *in, long long pos){
    int  pos_byte = pos / 8;
    short pos_bit = pos % 8;
    byte selected_byte = in[pos_byte];
    return ((selected_byte >> (7 - pos_bit)) & 0x1);
}

__device__ void set_bit_long_cu(byte *in, long long pos, short val) {
    int  pos_byte = pos / 8;
    short pos_bit = pos % 8;
    byte selected_byte = in[pos_byte];
    selected_byte = (byte)(((0xFF7F >> pos_bit) & selected_byte) & 0x00FF);
    byte new_byte = (byte)((val << (7 - pos_bit)) | selected_byte);
    in[pos_byte] = new_byte;
    return;
}

__device__ unsigned char get_bit_cu(byte *in, int pos){
    int pos_byte = pos / 8;
    short pos_bit = pos % 8;
    byte selected_byte = in[pos_byte];
    return ((selected_byte >> (7 - pos_bit)) & 0x1);
}

__device__ void set_bit_cu(byte *in, int pos, short val) {
    int pos_byte = pos / 8;
    short pos_bit = pos % 8;
    byte selected_byte = in[pos_byte];
    selected_byte = (byte)(((0xFF7F >> pos_bit) & selected_byte) & 0x00FF);
    byte new_byte = (byte)((val << (7 - pos_bit)) | selected_byte);
    in[pos_byte] = new_byte;
    return;
}


__global__ void generate_all_subkeys_cu(long long offset, byte *key, byte *sub_key, byte *c, byte *d, byte *cd, byte *copy){
    int tid = (blockIdx.x + gridDim.x * blockIdx.y) * blockDim.x + threadIdx.x;
    long long key_in_block = offset + tid;
    for (short i = 8; i < 64; ++i){
        short val = get_bit_from_longlong_cu(key_in_block, i);
        set_bit_cu(key, (tid * 7 * 8) + i - 8, val);
    }
    const short SHIFTS_LEN = 16;
    //select_bits_with_pos_in_key(*the_key, 0, PC1_LEN/2, &c);
    //byte copy[8];
    for (short i = 0; i < 7; ++i)
        copy[tid*8 + i] = key[(tid * 7) + i];
    
    for (short i = 0; i < 28; ++i){
        unsigned char val = get_bit_cu(copy, tid*8*8 + i);
        set_bit_cu(c, (tid * 4 * 8) + i, val);
    }
    //select_bits_with_pos_in_key(*the_key, PC1_LEN/2, PC1_LEN/2, &d);
    for (short i = 28; i < 56; ++i){
        unsigned char val = get_bit_cu(copy, tid*8*8 + i);
        set_bit_cu(d, (tid * 4 * 8) + i - 28, val);
    }
    
    for (short i = 0; i < 4; ++i)
        copy[tid*8 + i] = c[(tid * 4) + i];
    for (short i = 4; i < 8; ++i)
        copy[tid*8 + i] = d[(tid * 4) + i - 4];
    for (short cycle = 0; cycle < SHIFTS_LEN; ++cycle){
        //rotate_left(&c, 28, SHIFTS_cu[i]);
        for (short i = 0; i < 28; ++i) {
            short val = get_bit_cu(copy,tid*8*8 + (((i + SHIFTS_cu[cycle]) % 28)));
            set_bit_cu(c, (tid * 4 * 8) + i, val);
        }
        // rotate_left(&d, 28, SHIFTS[i]);
        for (short i = 32; i < 60; ++i) {
            short val = get_bit_cu(copy, tid*8*8 + (((i - 32 + SHIFTS_cu[cycle]) % 28)) + 32);
            set_bit_cu(d, (tid * 4 * 8) + i - 32, val);
        }
        
        for (short i = 0; i < 4; ++i)
            copy[tid*8 + i] = c[(tid * 4) + i];
        for (short i = 4; i < 8; ++i)
            copy[tid*8 + i] = d[(tid * 4) + i - 4];
        
        //concatenate_bits(c, 28, d, 28, &cd);
        int j = 0;
        for (int i = 0; i < 28; ++i){
            unsigned char val = get_bit_cu(c, (tid * 4 * 8) + i);
            set_bit_cu(cd, (tid * 7 * 8) + j, val);
            ++j;
        }
        
        for (int i = 0; i < 28; ++i){
            unsigned char val = get_bit_cu(d, (tid * 4 * 8) + i);
            set_bit_cu(cd, (tid * 7 * 8) + j, val);
            ++j;
        }
        //select_bits(&cd, PC2_cu, 48);
        for (short i = 0; i < 48; ++i){
            unsigned char val = get_bit_cu(cd, (tid * 7 * 8) + (PC2_cu[i] - 1));
            set_bit_long_cu(sub_key, (long long)(tid * 96LL * 8LL) + (long long)(cycle * 48LL) + (long long)i, val);
        }
        
    }
    
}







__device__ short get_bit_from_longlong_cu(long long in, short pos){
    return ((in >> (63 - pos)) & 0x1);
}



__global__ void try_all_cu(byte *sub_keys, byte *crypted_msg, byte *cipher_decrypted,
                           byte *left, byte *right, byte *left_right,
                           byte *r_backup, byte *r_commuted, long long *res, byte *copy_8 ){
    int tid = (blockIdx.x + gridDim.x * blockIdx.y) * blockDim.x + threadIdx.x;
    unsigned char partial = 0;
    
    while (partial < 3){
        /******************************* cipher_decrypt *****************************/
        for (short i = 0; i < 64; ++i){
            unsigned char val = get_bit_cu(crypted_msg, (partial * 64) + (IP_cu[i] - 1));
            set_bit_cu(cipher_decrypted, (tid*64) + i, val);
        }
        
        for (short i = 0; i < 32; ++i){
            unsigned char val = get_bit_cu(cipher_decrypted, (tid * 64) + i);
            set_bit_cu(left, (tid * 32) + i, val);
        }
        
        for (short i = 32; i < 64; ++i){
            unsigned char val = get_bit_cu(cipher_decrypted, (tid * 64) + i);
            set_bit_cu(right, (tid*32) + i - 32, val);
        }
        for (short key_counter = 0; key_counter < 16; ++key_counter){
            for (short copy_counter = 0; copy_counter < 4; ++copy_counter)
                r_backup[(tid * 4) + copy_counter] = right[(tid * 4) + copy_counter];
            for (short i = 0; i < 48; ++i){
                unsigned char val = get_bit_cu(right, (tid * 32) + (E_cu[i] - 1));
                set_bit_cu(r_commuted, (tid * 48) + i, val);
            }
            for (short i = 0; i < 6; ++i)
                copy_8[tid*8 + i] = r_commuted[(tid*6)+i];
            for (short i = 0; i < 6; ++i)
                r_commuted[(tid * 6)+i] = copy_8[tid*8 + i] ^ sub_keys[tid * 96 + 6 * (15 - key_counter) + i];
            for (short i = 0; i < 8; ++i){
                for (short j = 0; j < 6; ++j){
                    short val = get_bit_cu(r_commuted, (tid * 48)+(6 * i + j));
                    set_bit_cu(copy_8,tid*8*8 +  8 * i + j, val);
                }
            }
            
            int lh_byte = 0;
            for (short b = 0; b < 8; ++b) { // Should be sub-blocks
                byte val_byte = copy_8[tid*8 + b];
                short r = 2 * ((val_byte >> 7) & 0x0001) + ((val_byte >> 2) & 0x0001); // 1 and 6
                short c = (val_byte >> 3) & 0x000F; // Middle 4 bits
                short h_byte = S_cu[(64 * b) + (16 * r) + c]; // 4 bits (half byte) output
                if (b % 2 == 0) lh_byte = h_byte; // Left half byte
                else right[(tid * 4) + (b / 2)] = (byte)(16 * lh_byte + h_byte);
                /* * * * * * * * * * * * * * * * * * * * * * */
            }
            
            //select_bits(&right, P, 32);
            for (short i = 0; i < 32; ++i){
                unsigned char val = get_bit_cu(right, (tid * 32) + P_cu[i] - 1);
                set_bit_cu(copy_8, tid*8*8 + i, val);
            }
            for (short i = 0; i < 4; ++i)
                right[(tid * 4) + i] = copy_8[tid*8 + i];
            for (short i = 0; i < 4; ++i)
                right[(tid * 4) + i] = copy_8[tid*8 + i] ^ left[(tid * 4) + i];
            for (int i = 0; i < 4; ++i)
                left[(tid * 4) + i] = r_backup[(tid * 4) + i];
        }
        
        short j = 0;
        for (int i = 0; i < 32; ++i){
            unsigned char val = get_bit_cu(right, (tid * 32) + i);
            set_bit_cu(left_right, (tid * 64) + j, val);
            ++j;
        }
        for (int i = 0; i < 32; ++i){
            unsigned char val = get_bit_cu(left, (tid * 32) + i);
            set_bit_cu(left_right, (tid * 64) + j, val);
            ++j;
        }
        
        for (short i = 0; i < 64; ++i){
            unsigned char val = get_bit_cu(left_right, (tid * 64) + INVP_cu[i] - 1);
            set_bit_cu(cipher_decrypted, (tid * 64) + i, val);
        }
        for (short i = 0; i < 8; ++i)
            if (cipher_decrypted[(tid * 8) + i] != mess_known[(partial * 8) + i]){
                return;
            }
        ++partial;
        
    }
    *res = tid;
    return;
}

//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################
//##################################################################################################

extern "C"  void testOffsetGpu(byte **MODIFIED_MESSAGE, byte *SUCCESS,int processID){

int block[4]={1,1,1,1};
MPI_Datatype types[4]={MPI_LONG_LONG,MPI_LONG_LONG,MPI_DOUBLE,MPI_INT};
MPI_Aint disallineamentoBIT[4]={offsetof(infoExec,possibileChiave),offsetof(infoExec,processID),offsetof(infoExec,tempo),offsetof(infoExec,tipo)};
MPI_Type_struct(4, block, disallineamentoBIT, types, &Info_Type);
MPI_Type_commit(&Info_Type);
infoExec informazioni;
informazioni.tipo=0;
    short thread_number = 256 ;
    long long block_size = 16777216;
    long long cmc = -1;
    int num_gpus = 0;
    int exit_Condition=0;
    cudaGetDeviceCount(&num_gpus);
    omp_set_num_threads(num_gpus);
    //Calcoliamo il tempo
    informazioni.processID=processID;
    informazioni.possibileChiave=block_size;
//Calcoliamo il tempo

#pragma omp parallel
{

float tmp=0.0;
     	     byte *sub_keys, *key, *c, *d, *cd, *cipher_decrypted, *copy, *r_backup, *r_commuted, *left_right,*crypted_msg;
            long long *res;

            short tid = omp_get_thread_num();
            cudaSetDevice(tid);
            cudaMalloc((void **)&res, sizeof(long long));
            cudaMalloc((void **)&sub_keys, block_size * sizeof(byte) * 96);
            cudaMalloc((void **)&key, block_size * sizeof(byte) * 7);
            cudaMalloc((void **)&c, block_size * sizeof(byte) * 4);
            cudaMalloc((void **)&d, block_size * sizeof(byte) * 4);
            cudaMalloc((void **)&cd, block_size * sizeof(byte) * 7);
            cudaMalloc((void **)&copy, block_size * sizeof(byte) * 8);
            cudaMalloc((void **)&crypted_msg, sizeof(byte) * 24);
            cudaMemcpy(crypted_msg, *MODIFIED_MESSAGE, sizeof(byte)*24, cudaMemcpyHostToDevice);
            cudaMemcpy(res, &cmc, sizeof(long long), cudaMemcpyHostToDevice);
            cudaMalloc((void **)&cipher_decrypted, block_size * sizeof(byte) * 8);
            cudaMalloc((void **)&r_backup, block_size * sizeof(byte) * 4);
            cudaMalloc((void **)&r_commuted, block_size * sizeof(byte) * 6);
            cudaMalloc((void **)&left_right, block_size * sizeof(byte) * 8);
        
           long long d_offset;
            while(cmc == -1 && exit_Condition == 0 ){
#pragma omp critical  //Sezione Critica
                {
                    MPI_Send(&informazioni, sizeof(Info_Type), Info_Type, 0, 1, MPI_COMM_WORLD);
                    MPI_Recv(&d_offset, 1, MPI_LONG_LONG, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //Riceviamo il nuovo offset e lo testiamo
		if(d_offset==-1){
		       exit_Condition=-1;
		       d_offset=0;
		    }
                }
cudaEvent_t start, stop;
cudaEventCreate(&start);
cudaEventCreate(&stop);
cudaEventRecord(start);
                generate_all_subkeys_cu<<<dim3(thread_number, thread_number), thread_number>>>(d_offset, key, sub_keys, c, d, cd, copy);
                try_all_cu<<<dim3(thread_number, thread_number), thread_number>>>(sub_keys,crypted_msg,cipher_decrypted, c, d, left_right,r_backup,r_commuted,res, copy);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&tmp,start,stop);
                cudaMemcpy(&cmc, res, sizeof(long long), cudaMemcpyDeviceToHost);
                if (cmc != -1){
                    informazioni.possibileChiave=d_offset+cmc;
                    informazioni.tempo=tmp/1000;
                    MPI_Send(&informazioni, sizeof(Info_Type), Info_Type, 0, 0, MPI_COMM_WORLD); //Inviamo al server la chiave
                }
        informazioni.tempo=tmp/1000;


}
            cudaFree(key);
            cudaFree(c);
            cudaFree(d);
            cudaFree(cd);
            cudaFree(left_right);
            cudaFree(r_commuted);
            cudaFree(r_backup);
            cudaFree(cipher_decrypted);
            cudaFree(crypted_msg);
            cudaFree(sub_keys);
            cudaFree(res);
            cudaFree(copy);
        
    }
}




extern "C"  void testOffsetGpuJetson(byte *MODIFIED_MESSAGE, byte *SUCCESS,int processID){


int block[4]={1,1,1,1};
MPI_Datatype types[4]={MPI_LONG_LONG,MPI_LONG_LONG,MPI_DOUBLE,MPI_INT};
MPI_Aint disallineamentoBIT[4]={offsetof(infoExec,possibileChiave),offsetof(infoExec,processID),offsetof(infoExec,tempo),offsetof(infoExec,tipo)};
MPI_Type_struct(4, block, disallineamentoBIT, types, &Info_Type);
MPI_Type_commit(&Info_Type);
infoExec informazioni;
informazioni.tipo=0;

short thread_number = 256;
long long block_size = 16777216;
cudaSetDeviceFlags(cudaDeviceMapHost);
byte *sub_keys, *key, *c, *d, *cd, *cipher_decrypted, *r_backup, *r_commuted, *left_right, *crypted_msg, *copy;
byte *d_sub_keys, *d_key, *d_c, *d_d, *d_cd, *d_cipher_decrypted, *d_r_backup, *d_r_commuted, *d_left_right, *d_crypted_msg, *d_copy;
long long d_offset = 0;
long long *res;
long long *cmc;
cudaHostAlloc((void **)&cmc, sizeof(long long), cudaHostAllocMapped);
*cmc = -1;

informazioni.processID=processID;
informazioni.possibileChiave=block_size;

cudaHostGetDevicePointer((void **)&res, (void *)cmc, 0);

cudaHostAlloc((void **)&copy, block_size * sizeof(byte) * 8, cudaHostAllocMapped);
cudaHostGetDevicePointer((void **)&d_copy, (void *)copy, 0);

cudaHostAlloc((void **)&sub_keys, block_size * sizeof(byte) * 96, cudaHostAllocMapped);
cudaHostGetDevicePointer((void **)&d_sub_keys, (void *)sub_keys, 0);

cudaHostAlloc((void **)&key, block_size * sizeof(byte) * 7, cudaHostAllocMapped);
cudaHostGetDevicePointer((void **)&d_key, (void *)key, 0);

cudaHostAlloc((void **)&c, block_size * sizeof(byte) * 4, cudaHostAllocMapped);
cudaHostGetDevicePointer((void **)&d_c, (void *)c, 0);

cudaHostAlloc((void **)&d, block_size * sizeof(byte) * 4, cudaHostAllocMapped);
cudaHostGetDevicePointer((void **)&d_d, (void *)d, 0);

cudaHostAlloc((void **)&cd, block_size * sizeof(byte) * 7, cudaHostAllocMapped);
cudaHostGetDevicePointer((void **)&d_cd, (void *)cd, 0);
cudaHostAlloc((void **)&crypted_msg, sizeof(byte) * 24, cudaHostAllocMapped);
for (short i = 0; i<24; ++i){
crypted_msg[i] = MODIFIED_MESSAGE[i];
}
cudaHostGetDevicePointer((void **)&d_crypted_msg, (void *)crypted_msg, 0);
cudaHostAlloc((void **)&cipher_decrypted, block_size * sizeof(byte) * 8, cudaHostAllocMapped);
cudaHostGetDevicePointer((void **)&d_cipher_decrypted, (void *)cipher_decrypted, 0);
cudaHostAlloc((void **)&r_backup, block_size * sizeof(byte) * 4, cudaHostAllocMapped);
cudaHostGetDevicePointer((void **)&d_r_backup, (void *)r_backup, 0);
cudaHostAlloc((void **)&r_commuted, block_size * sizeof(byte) * 6, cudaHostAllocMapped);
cudaHostGetDevicePointer((void **)&d_r_commuted, (void *)r_commuted, 0);
cudaHostAlloc((void **)&left_right, block_size * sizeof(byte) * 8, cudaHostAllocMapped);
cudaHostGetDevicePointer((void **)&d_left_right, (void *)left_right, 0);

//Calcoliamo il tempo
cudaEvent_t start, stop;
cudaEventCreate(&start);
cudaEventCreate(&stop);


while(true){
    MPI_Send(&informazioni, sizeof(Info_Type), Info_Type, 0, 1, MPI_COMM_WORLD);//Comunichiamo al server che abbiamo finito la parte scatter e serve un nuovo offset da analizzare.
    MPI_Recv(&d_offset, 1, MPI_LONG_LONG, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //Riceviamo il nuovo offset e lo testiamo
    if(d_offset==-1){
        break;
    }
    cudaEventRecord(start);
    generate_all_subkeys_cu << <dim3(thread_number, thread_number), thread_number >> >(d_offset, d_key, d_sub_keys, d_c, d_d, d_cd, d_copy);
    cudaThreadSynchronize();
    try_all_cu << <dim3(thread_number, thread_number), thread_number >> >(d_sub_keys, d_crypted_msg, d_cipher_decrypted, d_c, d_d, d_left_right, d_r_backup, d_r_commuted, res, d_copy);
    cudaThreadSynchronize();
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float tmp=0.0;
    cudaEventElapsedTime(&tmp,start,stop);
informazioni.tempo=tmp/1000.0;
    if (*cmc != -1){
        informazioni.possibileChiave=d_offset+ *cmc;
        MPI_Send(&informazioni, sizeof(Info_Type), Info_Type, 0, 0, MPI_COMM_WORLD); //Inviamo al server la chiave
    break;
    }
}




cudaFreeHost(key);
cudaFreeHost(c);
cudaFreeHost(d);
cudaFreeHost(cd);

cudaFreeHost(left_right);
cudaFreeHost(r_commuted);
cudaFreeHost(r_backup);
cudaFreeHost(cipher_decrypted);
cudaFreeHost(sub_keys);
cudaFreeHost(res);
cudaFreeHost(crypted_msg);
cudaFreeHost(copy);
return;
}


