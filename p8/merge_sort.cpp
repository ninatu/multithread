#include <iostream>
#include <fstream>

using namespace std;

void
merge_sort(int *left, int*right)
{
    int len = right - left;
    if (len <= 1)
        return;
    else {
        int left_size = len / 2;
        int *start_left = left;
        int *end_left = left + left_size;
        int *start_right = end_left;
        int *end_right = right;

        #pragma omp parallel sections num_threads(2)
        {
            #pragma omp section
            { merge_sort(start_left, end_left);}
            #pragma omp section
            { merge_sort(start_right, end_right); }
        }
        // merge 2 parts
        int *tmp_array = new int[len];
        int *cur_tmp = tmp_array;

        while(start_left != end_left || start_right != end_right) {
            if (start_left == end_left) {
                *cur_tmp = *start_right;
                cur_tmp++;
                start_right++;
            } else if (start_right == end_right || *start_left < *start_right) {
                *cur_tmp = *start_left;
                cur_tmp++;
                start_left++;
            } else {
                *cur_tmp = *start_right;
                cur_tmp++;
                start_right++;
            }
        }
        for (int i = 0; i < len; i++)
            left[i] = tmp_array[i];
        delete []tmp_array;
    }
}


int
main(int argc, char **argv)
{
    if (argc > 3) {
        cout << "Error: enter 2 parameter - name_input_file, name_output_file" << endl;
        return 0;
    }
    std::fstream in_file(argv[1], std::fstream::in);
    std::fstream out_file(argv[2], std::fstream::out);
    int n, *array;

    in_file >> n;
    array = new int[n];

    for(int i = 0; i < n; i++) {
        in_file >> array[i];
    }


    merge_sort(array, array + n);
    for(int i = 0; i < n; i++) {
        out_file << array[i] << ' ';
    }
    delete []array;
    in_file.close();
    out_file.close();
}
