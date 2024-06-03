// Napisati konkurentni program koji simulira jednostavno zauzimanje radne memorije od strane procesa.
// Radna memorija je predstavljena datom klasom Radna_memorija i podeljena je na N okvira (N se prosleđuje pri instanciranju objekta klase Radna_memorija). Okvir predstavlja skup memorijskih lokacija u koji se mogu smestiti podaci ili instrukcije procesa.
//
// Procesi su predstavljeni nitima. Podaci i instrukcije procesa su podeljeni u stranice. Stranice procesa su jednake veličine kao okviri radne memorije. Da bi se proces izvršio, mora sve svoje stranice da učita u okvire radne memorije. Nije obavezno da stranice jednog procesa budu smeštene u susedne okvire radne memorije.
//
// Učitavanje stranica u okvire vrši se u metodi "ucitaj" klase Radna_memorija. Ukoliko u radnoj memoriji ne postoji dovoljan broj slobodnih okvira, proces će sačekati dok se potreban broj okvira ne oslobodi. Slobodnim okvirom se smatra okvir kojeg ni jedan proces ne koristi.
//
// Nakon izvršavanja procesa, proces treba da oslobodi okvire koje je koristio, što je potrebno implementirati u metodi "oslobodi".
//

#include <thread>
#include <random>

#include "radna_memorija.hpp"

using namespace std;
using namespace chrono;

// Funkcija niti koja simulira izvršenje procesa
void proces(Radna_memorija& mem, int id_procesa, int broj_stranica_procesa) {
	default_random_engine dr;
	dr.seed(steady_clock::now().time_since_epoch().count());
	uniform_int_distribution<int> trajanje_procesa(1, 3);

    mem.getDijagnostika().proces_se_ucitava(id_procesa, broj_stranica_procesa);

	mem.ucitaj(broj_stranica_procesa, id_procesa);

	// Simulacija izvršenja procesa:
	this_thread::sleep_for(seconds(trajanje_procesa(dr)));

	mem.getDijagnostika().proces_se_zavrsio(id_procesa);

	mem.oslobodi(id_procesa);
}

const int BROJ_OKVIRA = 4;        // Ukupan broj okvira u radnoj memoriji
const int UKUPNO_PROCESA = 10;    // Ukupan broj procesa koji se izvršavaju

void testirajSve() {
	default_random_engine dr;
	dr.seed(steady_clock::now().time_since_epoch().count());
	uniform_int_distribution<int> br_stranica_procesa(1, BROJ_OKVIRA);
    Dijagnostika d;
	Radna_memorija rm(d, BROJ_OKVIRA);

	thread procesi[UKUPNO_PROCESA];
	for (int i = 0; i < UKUPNO_PROCESA; ++i)
		procesi[i] = thread(proces, ref(rm), i + 1, br_stranica_procesa(dr));

	for (int i = 0; i < UKUPNO_PROCESA; ++i)
		procesi[i].join();
}

int main() {
    testirajSve();

    return 0;
}
