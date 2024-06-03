#ifndef NAREDBA_HPP_INCLUDED
#define NAREDBA_HPP_INCLUDED

#include <string>

using namespace std;

struct Naredba {				// Struktura koja modeluje 2 tipa naredbe - ili malloc ili repeat
	string tip;
	int ponavljanja;
	int kolicina_memorije;
	Naredba(string t, int p, int k_m) : tip(t), ponavljanja(p), kolicina_memorije(k_m) {}
};

#endif // NAREDBA_HPP_INCLUDED
