#include <iostream>
#include <string>

using namespace std;

int main() {
    string token;

    cout << "Masukkan token: ";
    cin >> token;

    if (token == "indra-pramudya-123") {
        cout << "Selamat menikmati aplikasi kami!" << endl;
    } else {
        cout << "Token ditolak!" << endl;
    }

    return 0;
}