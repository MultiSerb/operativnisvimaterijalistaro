// Napisati konkurentni program koji simulira prioritetni raspoređivač u jednoprocesorskom sistemu.
//
// Procesi su predstavljeni nitima. Da bi se izvršili pozivaju metodu "izvrsi" deljene promenljive tipa Rasporedjivac. Svaki proces sadrži određeni broj naredbi. Naredbe se izvršavaju jedna po jedna. Za svaku naredbu, proces mora da sačeka da procesor bude slobodan. Izvršavanje jedne naredbe traje 300 ms. Nakon izvršavanja naredbe, proces odlazi u stanje spreman, a drugi spreman proces dobija procesor.
//
// Procesi su klasifikovani prema prioritetu. 0 je najviši prioritet dok je N najmanji!
//
// Za svaki prioritet, postoji odvojen red čekanja na procesor. Prednost u korišćenju procesora imaju procesi sa višim prioritetom. Među procesima istog prioriteta, raspoređivanje se obavlja po FIFO principu (tj. ciklično će se smenjivati različite naredbe procesa istih prioriteta na procesoru).
//
// Prioritet procesa je određen statički u trenutku kreiranja procesa.
//
// Za uspostavljanje reda čekanja po FIFO principu koristiti datu klasu "cv_hrono".
//

#include <thread>
#include <random>

#include "rasporedjivac.hpp"

using namespace std;

const int BROJ_PROCESA = 5;
const int BROJ_PRIORITETA = 3;

// Funkcija koju koriste niti koje simuliraju izvršenje procesa
void proces(Rasporedjivac& r, int id_procesa, int broj_naredbi) {
    default_random_engine dr;
    dr.seed(chrono::steady_clock::now().time_since_epoch().count());
    uniform_int_distribution<int> raspodela_prioriteta(0, BROJ_PRIORITETA-1);

    int prioritet = raspodela_prioriteta(dr);

    r.getDijagnostika().proces_kreiran(id_procesa, prioritet, broj_naredbi);

	r.izvrsi(id_procesa, broj_naredbi, prioritet);

	r.getDijagnostika().proces_zavrsio(id_procesa, prioritet);
}

void testirajSve() {
    default_random_engine dr;
    dr.seed(chrono::steady_clock::now().time_since_epoch().count());
    uniform_int_distribution<int> raspodela_br_naredbi(1, 10);

    Dijagnostika d;
    Rasporedjivac r(d, BROJ_PRIORITETA);
    thread procesi[BROJ_PROCESA];

    for (int i = 0; i < BROJ_PROCESA; i++)
	   procesi[i] = thread(proces, ref(r), i+1, raspodela_br_naredbi(dr));

    for (thread &proces : procesi)
	   proces.join();
}

int main() {
    testirajSve();

    return 0;
}
