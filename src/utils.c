#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

void fisher_yates_shuffle(int arr[], int size) {
    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}
