#ifndef DIJAGNOSTIKA_HPP_INCLUDED
#define DIJAGNOSTIKA_HPP_INCLUDED

#include <iostream>

using namespace std;

class Dijagnostika {
private:
    mutex term_mx;
public:
    // id     - id procesa
    // adresa - adresa sa koje je pokušano čitanje
    void proces_ceka_citanje(int id, int adresa) {
        unique_lock<mutex> l(term_mx);
        cout << "[PROCES] " << id << " ceka kako bi izvrsio citanje sa adrese " << adresa << endl;
    }

    // id     - id procesa
    // adresa - adresa na koju je pokušan upis
    void proces_ceka_upis(int id, int adresa) {
        unique_lock<mutex> l(term_mx);
        cout << "[PROCES] " << id << " ceka kako bi izvrsio upis na adresu " << adresa << endl;
    }

    // id       - id procesa
    // adresa   - adresa sa koje očitan karakter
    // vrednost - karakter koji je pročitan
    void proces_procitao(int id, int adresa, char vrednost) {
        unique_lock<mutex> l(term_mx);
        cout << "[PROCES] " << id << " PROCITAO sa adrese " << adresa << " vrednost '" << vrednost << "'" << endl;
    }

    // id       - id procesa
    // adresa   - adresa na koju je upisana vrednost
    // vrednost - karakter koji je upisan
    void proces_upisao(int id, int adresa, char vrednost) {
        unique_lock<mutex> l(term_mx);
        cout << "[PROCES] " << id << " UPISAO na adresu " << adresa << " vrednost '" << vrednost << "'" << endl;
    }
};

#endif // DIJAGNOSTIKA_HPP_INCLUDED
