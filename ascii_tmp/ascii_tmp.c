#include<iostream>
#include<iomanip>
using namespace std;

int main()
{
    cout << " ASCII 32 - 255\n";

    unsigned char myarray[7][32] = {};
    int val = 32;
    {

        for (size_t i = 0; i < 7; i++) {
            for (size_t j = 0; j < 32; j++) {
                myarray[i][j] = val++;
            }
        }
    }
    for (int asc_char = 32; asc_char < 256; asc_char++)
        //  cout << std::setw(6) << asc_char << setw(3) << static_cast<char>(asc_char);
        for (auto& row : myarray) {
            for (auto& col : row) {
                std::cout << col ;
            }
            std::cout << '\n';
        }

    return 0;
}
