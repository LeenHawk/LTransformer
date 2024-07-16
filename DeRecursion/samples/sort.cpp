#include <stdint.h>
void r_sort(uint32_t *a, int n)
{
    int split=0;

    if( n < 32 ){

        // From Rosetta Code
        // http://rosettacode.org/wiki/Sorting_algorithms/Insertion_sort#C
        /*fprintf(stderr, "n=%d\n", n);
        for(int ii=0; ii<n;ii++){
            fprintf(stderr, " %.5f", a[ii]);
        }
        fprintf(stderr, "\n");*/
        int i, j;
        uint32_t t;
        for (i = 1; i < n; i++) {
            t = a[i];
            for (j = i; j > 0 && t < a[j - 1]; j--) {
                a[j] = a[j - 1];
            }
            a[j] = t;
            /*for(int ii=0; ii<n;ii++){
                fprintf(stderr, " %.5f", a[ii]);
            }
            fprintf(stderr, "\n");*/
        }
    }else{
       // fprintf(stderr, "n=%d  ''\n", n);
        // This is taken from Rosetta Code
        // http://rosettacode.org/wiki/Sorting_algorithms/Quicksort#C
        uint32_t pivot=a[n/2];
        int i,j;
        for(i=0, j=n-1;; i++, j--){
            while(a[i]<pivot)
                i++;
            while(pivot<a[j])
                j--;
            if(i>=j)
                break;
            uint32_t tmp=a[i];
            a[i]=a[j];
            a[j]=tmp;
        }
        split=i;
        r_sort(a,       split);
        r_sort(a+split, n-split);
    }
}
