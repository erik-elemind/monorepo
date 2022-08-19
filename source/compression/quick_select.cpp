
#include <quick_select.h>



/*
 * Exploration of Quick Select implementations, as described by the blog post:
 * https://softwareengineering.stackexchange.com/questions/284767/kth-selection-routine-floyd-algorithm-489
 */
	
CMPR_FLOAT select7MO3_ascending(CMPR_FLOAT* array, const int length, const int kTHvalue)
{
#define F_SWAP(a,b) { CMPR_FLOAT temp=(a);(a)=(b);(b)=temp; }
#define SIGNUM(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : (x)))
    int left = 0;
    int i;
    int right = length - 1;

    while (right > left)
    {
        if( array[kTHvalue] < array[left] ) F_SWAP(array[left], array[kTHvalue]);
        if( array[right] < array[left] ) F_SWAP(array[left], array[right]);
        if( array[right] < array[kTHvalue] ) F_SWAP(array[kTHvalue], array[right]);

        // partition the elements between left and right around t
        CMPR_FLOAT t = array[kTHvalue];
        i = left;
        int j = right;

        F_SWAP(array[left], array[kTHvalue]);

        if (array[right] > t)
        {
            F_SWAP(array[right], array[left]);
        }

        while (i < j)
        {
            F_SWAP(array[i], array[j]);
            i++;
            j--;

            while (array[i] < t)
            {
                i++;
            }

            while (array[j] > t)
            {
                j--;
            }
        }

        if (array[left] == t)
        {
            i--;
            F_SWAP (array[left], array[j]);
        }             
        else
        {
            j++;
            F_SWAP (array[j], array[right]);
        }

        // adjust left and right towards the boundaries of the subset
        // containing the (k - left + 1)th smallest element
        if (j <= kTHvalue)
        {
            left = j + 1;
        }
        else if (kTHvalue <= j)
        {
            right = j - 1;
        }

    }
    
#undef F_SWAP
#undef SIGNUM
    return array[kTHvalue];
}

CMPR_FLOAT select7MO3_descending(CMPR_FLOAT* array, const int length, const int kTHvalue)
{
#define F_SWAP(a,b) { CMPR_FLOAT temp=(a);(a)=(b);(b)=temp; }
#define SIGNUM(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : (x)))
    int left = 0;
    int i;
    int right = length - 1;
    
    while (right > left)
    {
        if( array[kTHvalue] > array[left] ) F_SWAP(array[left], array[kTHvalue]);
        if( array[right] > array[left] ) F_SWAP(array[left], array[right]);
        if( array[right] > array[kTHvalue] ) F_SWAP(array[kTHvalue], array[right]);

        // partition the elements between left and right around t
        CMPR_FLOAT t = array[kTHvalue];
        i = left;
        int j = right;

        F_SWAP(array[left], array[kTHvalue]);

        if (array[right] < t)
        {
            F_SWAP(array[right], array[left]);
        }

        while (i < j)
        {
            F_SWAP(array[i], array[j]);
            i++;
            j--;

            while (array[i] > t)
            {
                i++;
            }

            while (array[j] < t)
            {
                j--;
            }
        }

        if (array[left] == t)
        {
            i--;
            F_SWAP (array[left], array[j]);
        }             
        else
        {
            j++;
            F_SWAP (array[j], array[right]);
        }

        // adjust left and right towards the boundaries of the subset
        // containing the (k - left + 1)th smallest element
        if (j <= kTHvalue)
        {
            left = j + 1;
        }
        else if (kTHvalue <= j)
        {
            right = j - 1;
        }

    }
    
#undef F_SWAP
#undef SIGNUM
    return array[kTHvalue];
}
	

CMPR_FLOAT FloydWirth_kth_ascending(CMPR_FLOAT* arr, const int length, const int kTHvalue)
{
#define F_SWAP(a,b) { CMPR_FLOAT temp=(a);(a)=(b);(b)=temp; }
#define SIGNUM(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : (x)))

    int left = 0;       
    int right = length - 1;     
    int left2 = 0;
    int right2 = length - 1;

    while (left < right) 
    {           
        if( arr[right2]   < arr[left2]    ) F_SWAP(arr[left2], arr[right2]);
        if( arr[right2]   < arr[kTHvalue] ) F_SWAP(arr[kTHvalue], arr[right2]);
        if( arr[kTHvalue] < arr[left2]    ) F_SWAP(arr[left2], arr[kTHvalue]);

        double x=arr[kTHvalue];

        while ((right2 > kTHvalue) && (left2 < kTHvalue))
        {
            do 
            {
                left2++;
            }while (arr[left2] < x);

            do
            {
                right2--;
            }while (arr[right2] > x);

            F_SWAP(arr[left2], arr[right2]);
        }
        left2++;
        right2--;

        if (right2 < kTHvalue) 
        {
            while (arr[left2]<x)
            {
                left2++;
            }
            left = left2;
            right2 = right;
        }

        if (kTHvalue < left2) 
        {
            while (x < arr[right2])
            {
                right2--;
            }

            right = right2;
            left2 = left;
        }

        if( arr[left] < arr[right] ) F_SWAP(arr[right], arr[left]);
    }

#undef F_SWAP
#undef SIGNUM
    return arr[kTHvalue];
}

CMPR_FLOAT FloydWirth_kth_descending(CMPR_FLOAT* arr, const int length, const int kTHvalue)
{
#define F_SWAP(a,b) { CMPR_FLOAT temp=(a);(a)=(b);(b)=temp; }
#define SIGNUM(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : (x)))

    int left = 0;       
    int right = length - 1;     
    int left2 = 0;
    int right2 = length - 1;

    while (left < right) 
    {           
        if( arr[right2]   > arr[left2]    ) F_SWAP(arr[left2], arr[right2]);
        if( arr[right2]   > arr[kTHvalue] ) F_SWAP(arr[kTHvalue], arr[right2]);
        if( arr[kTHvalue] > arr[left2]    ) F_SWAP(arr[left2], arr[kTHvalue]);

        double x=arr[kTHvalue];

        while ((right2 > kTHvalue) && (left2 < kTHvalue))
        {
            do 
            {
                left2++;
            }while (arr[left2] > x);

            do
            {
                right2--;
            }while (arr[right2] < x);

            F_SWAP(arr[left2], arr[right2]);
        }
        left2++;
        right2--;

        if (right2 < kTHvalue) 
        {
            while (arr[left2]>x)
            {
                left2++;
            }
            left = left2;
            right2 = right;
        }

        if (kTHvalue < left2) 
        {
            while (x > arr[right2])
            {
                right2--;
            }

            right = right2;
            left2 = left;
        }

        if( arr[left] > arr[right] ) F_SWAP(arr[right], arr[left]);
    }

#undef F_SWAP
#undef SIGNUM
    return arr[kTHvalue];
}

