#ifndef RASPOREDJIVAC_H_INCLUDED
#define RASPOREDJIVAC_H_INCLUDED

#include <vector>
#include <list>
#include <mutex>
#include <condition_variable>
#include <random>

#include "dijagnostika.h"
#include "cv_tag.h"

using namespace std;

struct Proces {
	int id;
	int broj_naredbi;
	int broj_odsecaka;
	Proces(int i, int b, int o) : id(i), broj_naredbi(b), broj_odsecaka(o) {}
};

class Rasporedjivac {
private:
	Dijagnostika& dijagnostika;
   	int aktivni_proces;					// ID aktivnog procesa
	cv_tag red;							// CV sa privescima. Omogućava aktivno obaveštavanje po privesku.
	mutex m;
	list<int> proces_ids;			//lista koja sadrzi ID-eve procesa. Svaki proces imace onoliko elemenata
									//koliko ima odsecaka
    default_random_engine dr;
public:
    Rasporedjivac(Dijagnostika& d) : dijagnostika(d) {
        dr.seed(steady_clock::now().time_since_epoch().count());
        aktivni_proces = -1;
    }

    Dijagnostika& getDijagnostika() {
        return dijagnostika;
    }

    // Metoda koju poziva nit koja simulira izvršenje procesa, kako bi se proces izvršio
    //
    // p - struktura koja sadrži ID procesa, broj naredbi koje treba da izvrši kao i broj odsecaka.
    //
    // Ukoliko je procesor već zauzet i ne mogu se izvršavati naredbe procesa, potrebno je pozvati metodu dijagnostika.proces_ceka a nakon toga proces treba da pređe u stanje čekanja.
    // Nakon što je proces izvršio naredbu, potrebno je pozvati dijagnostika.izvrsio_naredbu.
	void izvrsi(Proces p) {
		// Dodaju se id-evi ovog procesa u listu svih id-eva (thread-safe operacija):
        {
            unique_lock<mutex> l(m);
            for (int i = 0; i < p.broj_odsecaka; i++) {
                proces_ids.push_back(p.id);
            }
        }

 		for (int i = 0; i < p.broj_naredbi; ++i) {  // Kreće izvršavanje naredbi tek NAKON dodavanja id procesa u listu
            unique_lock<mutex> l(m);
            // Ako je procesor slobodan, zauzima se:
            if (aktivni_proces == -1) {
                aktivni_proces = p.id;
            }

            // Ako neko drži procesor, čeka se u cv_tag uvezujući privezak koji ima vrednost id procesa:
            while (aktivni_proces != p.id) {
                dijagnostika.proces_ceka(p.id);
                red.wait(l, p.id);
            }

            l.unlock();
            this_thread::sleep_for(chrono::milliseconds(300));	// Simulacija izvršenja naredbe
            l.lock();

 			// Ako je ovo poslednja instrukcija procesa, njegovi id-evi se izbacuju liste:
            if (i == p.broj_naredbi - 1) {
				proces_ids.remove(p.id);
            }

            if (!proces_ids.empty()) {
                uniform_int_distribution<int> raspodela_indeksa_pidova(0, proces_ids.size()-1);
                int poz_ida = raspodela_indeksa_pidova(dr);
                // Naredni proces je onaj koji je indeksiran:
                auto it = proces_ids.begin();
                for (int i = 0; i < poz_ida; i
                ++, it++);  // Pomeranje iteratora do pozicije ida
                aktivni_proces = *it;
                // Budi se proces id procesa je privezak u cv_tag promenljivoj red:
                red.notify(aktivni_proces);
            } else {
                aktivni_proces = -1;
            }

            dijagnostika.izvrsio_naredbu(p.id, i, aktivni_proces);
        }
	}
};

#endif // RASPOREDJIVAC_H_INCLUDED
