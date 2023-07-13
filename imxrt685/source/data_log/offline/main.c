#include "data_log_parse.h"

int main(void)
{
    // Test binaries can be found here:
    // https://drive.google.com/drive/u/0/folders/1cAaZ5PYbW1EgcpQkPWUwzmO9yHNjoQL_

    // small file
    data_log_parse("DataLogSamples/log9_2021_09_17_22_05_11_EDT.bin");
    // big file:
    // data_log_parse("DataLogSamples/log18_2021_09_28_21_40_35_EDT.bin");
}