#ifndef DIJAGNOSTIKA_H_INCLUDED
#define DIJAGNOSTIKA_H_INCLUDED

#include "tip_procesa.h"

#include <iostream>
#include <mutex>
#include <vector>

using namespace std;

class Dijagnostika {

private:
    mutex term_mx;

public:

    // parametar id - jedinstveni identifikator procesa
    // parametar broj_stranica_procesa - koliko stranica/okvira treba procesu za izvršavanje
    // parametar tip - da li je proces USER ili SYSTEM
    void proces_pokrenut (int id, int broj_stranica_procesa, Tip_procesa::tip_procesa tip){
        unique_lock<mutex> l(term_mx);
        cout << "[ ID: " << id << " ] ";
            tip == Tip_procesa::USER ? cout << "USER" : cout << "SYSTEM";
        cout << " proces sa " << broj_stranica_procesa << " stranica, pokusava da se ucita u radnu memoriju." << endl;
    }

    // parametar id - jedinstveni identifikator procesa
    // parametar broj_stranica_procesa - koliko stranica/okvira treba procesu za izvršavanje
    // parametar tip - da li je proces USER ili SYSTEM
    void proces_ceka (int id, int broj_stranica_procesa, Tip_procesa::tip_procesa tip){
        unique_lock<mutex> l(term_mx);
        cout << "[ ID: " << id << " ] ";
            tip == Tip_procesa::USER ? cout << "USER" : cout << "SYSTEM";
        cout << " proces sa " << broj_stranica_procesa << " stranica, ceka da se ucita u radnu memoriju." << endl;
    }

    // parametar id - jedinstveni identifikator procesa
    // parametar broj_stranica_procesa - koliko stranica/okvira treba procesu za izvršavanje
    // parametar tip - da li je proces USER ili SYSTEM
    void proces_zauzeo (int id, int broj_stranica_procesa, Tip_procesa::tip_procesa tip) {
		unique_lock<mutex> l(term_mx);
		cout << "[ ID: " << id << " ] ";
            tip == Tip_procesa::USER ? cout << "USER" : cout << "SYSTEM";
		cout<< " proces ("<< broj_stranica_procesa << " okvira) je zauzeo okvire."<<endl;
	}

    // parametar id - jedinstveni identifikator procesa
    // parametar tip - da li je proces USER ili SYSTEM
	void proces_oslobodio (int id, Tip_procesa::tip_procesa tip) {
        unique_lock<mutex> l(term_mx);
        cout << "[ ID: " << id << " ] ";
            tip == Tip_procesa::USER ? cout << "USER" : cout << "SYSTEM";
        cout<< " proces oslobodio radnu memoriju." << endl;
    }

    // Ova metoda prikazuje sve indekse okvira koji su postavljeni u kolekciju kojoj se pristupa pomoću iteratora pocetak i kraj
    //
    // pocetak - iterator na prvi element kolekcije koja sadrži indekse okvira koje treba ispisati
    // kraj    - iterator iza poslednjeg elementa kolekcije koja sadrži indekse okvira koje treba ispisati
	void ispisi_okvire(vector<int>::iterator pocetak, vector<int>::iterator kraj) {
        unique_lock<mutex> l(term_mx);
        cout << "{";

        int c = 0;
        for (vector<int>::iterator it = pocetak; it != kraj; it++) {
            if (c++ > 0)
                cout << " ";
            cout << *it;
        }

        cout << "}" << endl;
    }

};


#endif // DIJAGNOSTIKA_H_INCLUDED
