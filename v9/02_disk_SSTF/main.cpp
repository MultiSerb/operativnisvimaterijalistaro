// Napisati konkurentni program koji simulira raspoređivanje diska po SSTF
// (Shortest Service Time First) principu.
//
// Kod ovog načina raspoređivanja diska, iz skupa UI zahteva prednost ima onaj
// zahtev koji zahteva najmanje pomeranje ručice diska. Drugim rečima,
// pristupiće se onoj stazi koja je najbliža stazi na kojoj je trenutno glava
// diska.
//
// Klasa OS predstavlja deo operativnog sistema koji je zadužen za
// raspoređivanje zahteva. Metodu uputi_ui_zahtev pozivaju niti koje
// predstavljaju procese. U ovoj metodi postavlja se zahtev raspoređivaču diska
// da izvrši UI operaciju na određenoj stazi diska. Proces mora da sačeka da
// zahtev bude izvršen, pre nego nastavi dalje izvršavanje.
//
// Metodu obradi_zahtev poziva nit koja predstavlja raspoređivač diska. Metoda
// je zadužena da obradi jedan zahtev iz skupa zahteva. Zahtev za obradu se iz
// skupa zahteva bira po SSTF principu. Da bi se zahtev obradio, glava mora
// da stigne nad stazu na kojoj se nalaze trazeni podaci. Obrada zahteva traje
// 10*(razlika izmedju trenutne i staze zahteva) ms. Rezultat metode je broj
// staze kojoj je disk pristupio pri obradi zahteva.
//
// Takodje, potrebno je sumirati vremena obrada zahteva i implementirati metodu
// koja vraca ukupno utroseno vreme u ms.
//

#include <thread>

#include "os.hpp"

using namespace std;
using namespace chrono;

// Funkcija niti koju koriste niti koje simuliraju izvršenje procesa:
void proces(OS& os, int broj_staze, int rbp) {
	os.getDijagnostika().zahtev_prosledjen(rbp, broj_staze);
	os.uputi_UI_zahtev(rbp, broj_staze);
}

// Funkcija niti koju koristi nit koja simulira deo operativnog sistema koji upravlja diskom:
void rasporedjivac_diska(OS& os) {
	this_thread::sleep_for(milliseconds(200)); // Čekanje da pristignu svi zahtevi
    os.getDijagnostika().uredjaj_aktiviran();
	while (true) {
		int broj_staze = os.obradi_zahtev();
		if (broj_staze == -1) break;
		os.getDijagnostika().zahtev_obradjen(broj_staze);
	}
}

void testirajSve() {
	int brojevi_staza[] = {55, 58, 39, 18, 90, 160, 150, 38, 184};

    Dijagnostika d;
	OS os(d, 100);  // Inicijalno na stazi 100

	thread rasporedjivac_thread(rasporedjivac_diska, ref(os));
	rasporedjivac_thread.detach();

	thread procesi[9];
	for (int i = 0; i < 9; i++)
		procesi[i] = thread(proces, ref(os), brojevi_staza[i], i+1);

	for (int i = 0; i < 9; i++)
		procesi[i].join();

	os.zavrsi();

	cout << "Obrada svih zahteva je trajala: " << os.dobaviUkupnoTrajanje() << "ms." << endl;
}

int main() {
    testirajSve();

	return 0;
}
