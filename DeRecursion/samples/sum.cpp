#include <stdint.h>
void r_sum(uint32_t n, int32_t *f)
{
    if(n<=1){
        return;
    }else{
        r_sum(n/2, f);
        r_sum(n-(n/2), f+n/2);
        f[0] += f[n/2];
    }
};