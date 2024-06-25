#define N 32
#define n 5
// int in[32] = {0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1};

void polarencode(int in[32]) {  
    // int p;
    for (int s = 0; s < n; s++){
        int GP_num = 1 << s;
        int GP_size = N >> s;
        int GP_half_size = GP_size >> 1;
        int curr_GP_base_addr = 0;
        for (int GP_idx = 0; GP_idx < GP_num; GP_idx++){
            for (int t = 0; t < GP_half_size; t++){
                in[curr_GP_base_addr + t] ^= in[curr_GP_base_addr + t + GP_half_size];
            }
            curr_GP_base_addr += GP_size;
        }
    }
    // p=1;
}

