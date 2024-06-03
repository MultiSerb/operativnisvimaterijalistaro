// Tekst zadatka je u PDF fajlu.

#include <thread>

#include "os.h"

using namespace std;
using namespace chrono;

// Funkcija niti koju koriste niti koje simuliraju izvršenje procesa:
void proces(OS& os, int broj_staze, int rbp) {
	os.getDijagnostika().zahtev_prosledjen(rbp, broj_staze);
	os.uputi_UI_zahtev(rbp, broj_staze);
}

// Funkcija niti koju koristi nit koja simulira deo operativnog sistema koji upravlja diskom:
void rasporedjivac_diska(shared_ptr<OS> os) {
	this_thread::sleep_for(milliseconds(200)); // Čekanje da pristignu svi zahtevi
    os->getDijagnostika().uredjaj_aktiviran();
	while (true) {
		int broj_staze = os->obradi_zahtev();
		os->getDijagnostika().zahtev_obradjen(broj_staze);
	}
}

void testirajSve() {
	int brojevi_staza[] = {98, 183, 37, 122, 14, 124, 65, 67, 44};

    Dijagnostika d;
	shared_ptr<OS> os = make_shared<OS>(d, 53, true);  // Inicijalno na stazi 53, smer kretanja naniže - false

	thread rasporedjivac_thread(rasporedjivac_diska, os);
	rasporedjivac_thread.detach();

	thread procesi[9];
	for (int i = 0; i < 9; i++)
		procesi[i] = thread(proces, ref(*os), brojevi_staza[i], i+1);

	for (int i = 0; i < 9; i++)
		procesi[i].join();
}

int main() {
    testirajSve();

	exit(0);  //exit 0 - predstavlja signal detach-ovanoj niti da prekine rad jer više nije neophodna
}
