#include <iostream>
#include <string>

using namespace std;

int main() {
    string token;

    cout << "Masukkan token: ";
    cin >> token;

    if (token == "recoon-1337") {
        cout << "Selamat menikmati aplikasi kami!" << endl;
    } else {
        cout << "Token ditolak!" << endl;
    }

    return 0;
}