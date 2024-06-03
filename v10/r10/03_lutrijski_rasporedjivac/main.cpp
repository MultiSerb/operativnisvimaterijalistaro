// Napisati konkurentni program koji simulira lutrijski raspoređivač u jednoprocesorskom sistemu.
//
// Procesi su predstavljeni nitima. Da bi se izvršili pozivaju metodu "izvrsi" deljene promenljive tipa Rasporedjivac. Svaki proces sadrži određeni broj naredbi. Naredbe se izvršavaju jedna po jedna. Za svaku naredbu, proces mora da sačeka da procesor bude slobodan. Izvršavanje jedne naredbe traje 300 ms. Nakon izvršavanja naredbe, proces odlazi u stanje spreman, a drugi spreman proces dobija procesor.
//
// Lutrijski raspoređivač se zasniva na tome da je svakom procesu inicijalno dodeljen određeni broj tiketa. Pri izboru sledećeg procesa za izvršavanje, na slučajan način se bira jedan tiket. Proces kojem je dodeljen taj tiket postaje aktivan. Dodelom različitog broja tiketa procesima se implicitno uspostavljaju priroriteti među njima, obzirom da proces sa većim brojem tiketa ima veće šanse da bude izabran.
//

#include <vector>
#include <thread>
#include <random>

#include "rasporedjivac.hpp"

using namespace std;

void proces(Rasporedjivac& r, vector<int> tiketi_procesa, int id_procesa, int broj_naredbi) {
    Proces p(id_procesa, broj_naredbi, tiketi_procesa);
    r.getDijagnostika().proces_kreiran(id_procesa, broj_naredbi, tiketi_procesa.begin(), tiketi_procesa.end());

    r.izvrsi(p);

    r.getDijagnostika().proces_zavrsio(id_procesa);
}

const int BROJ_PROCESA = 5;

void testirajSve() {
    default_random_engine dr;
    dr.seed(chrono::steady_clock::now().time_since_epoch().count());
    uniform_int_distribution<int> raspodela_tiketa(1, 4007);
    uniform_int_distribution<int> raspodela_br_tiketa_po_procesu(1, 10);
    uniform_int_distribution<int> raspodela_br_naredbi(1, 5);

	// Baza svih mogućih tiketa sadrži tikete od 1 do 100.
	// Deo ovih tiketa će biti dodeljen procesima.
	vector<int> baza_tiketa;
	for (int i = 0; i < 100; i++) {
        baza_tiketa.push_back(i+1);
	}

    Dijagnostika d;
	Rasporedjivac r(d);
	thread procesi[BROJ_PROCESA];

	for (int i = 0; i < BROJ_PROCESA; i++) {
        // Na slučajan način se dodeljuje određeni broj tiketa procesu:
        vector<int> tiketi_procesa;
        int br_tiketa = raspodela_br_tiketa_po_procesu(dr);

        for (int i = 0; i < br_tiketa; i++) {
            int poz = raspodela_tiketa(dr) % baza_tiketa.size(); // Bilo koji slobodan tiket
            tiketi_procesa.push_back(baza_tiketa[poz]);
            baza_tiketa.erase(baza_tiketa.begin() + poz); // Ovaj tiket je dodeljen pa se izbacuje (ne sme biti dupliranih tiketa)
        }
		procesi[i] = thread(proces, ref(r), tiketi_procesa, i+1, raspodela_br_naredbi(dr));
    }

	for (int i = 0; i < BROJ_PROCESA; i++)
		procesi[i].join();
}

int main() {
    testirajSve();

    return 0;
}
