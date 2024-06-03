// Napisati konkurentni program koji modeluje izvršavanje procesa u multiprogramiranom jednoprocesorskom okruženju.
//
// Procesi su predstavljeni nitima. Svaki proces ima skup instrukcija koje treba da izvrši, pri čemu postoje dva tipa instrukcija: STANDARD (instrukcija zahteva određeno izračunavanje od procesora) i UI (zahtev za operacijom ulazno/izlaznog uređaja). U ovom zadatku, smer komunikacije sa UI uređajem je takav da proces inicira UI operaciju i pritom ostaje blokiran dok se ta operacija ne završi.
//
// Procesor je incijalno slobodan i zauzima ga prvi proces koji do njega dođe i pokuša da izvrši instrukciju.
// Sledi opis obrade STANDARD i UI instrukcija.
//
//-------------------------------------------------------------------------------------------------------------------
// OPIS OBRADE STANDARD INSTRUKCIJE:
//
// U sistemu je definisan maksimalan broj od 3 STANDARD instrukcije koje proces koji je zauzeo procesor može uzastopno da izvrši. Nakon što izvrši 3 STANDARD instrukcije, proces prelazi u stanje spreman a drugi proces iz liste spremnih dobija procesor. Na ovaj način na procesoru se vrši preključivanje između različitih procesa i omogućava se ravnomerna raspodela procesorskog vremena.
//
// Pri preključivanju nakon 3 STANDARD instrukcije treba voditi računa da se obezbedi mehanizam da proces koji oslobađa procesor (a koji ima još preostalih instrukcija za izvršavanje) ne pokuša da zauzme procesor ODMAH nakon puštanja. Ovo se postiže uvođenjem cooldown perioda od 100ms.
//
// Iz liste spremnih procesa, naredni proces za izvršavanje se bira bez određenog pravila. Bilo koji od spremnih procesa prelazi u izvršavanje.
//
// Ukoliko neki proces nema više instrukcija, on završava svoj rad i prepušta procesor nekom od procesa iz liste spremnih procesa.
//
// Izvršavanje STANDARD instrukcije traje slučajan vremenski period između 10 i 50 ms čime se npr. simulira aritmetičko-logička operacija na procesoru.
//
//--------------------------------------------------------------------------------------------------------------------
//
// OPIS OBRADE UI INSTRUKCIJE:
//
// UI instrukcije predstavljaju zahteve za operacijom ulazno izlaznih uređaja i njih obrađuje UI podsistem (preko niti ui_podsistem). UI podsistem REDOM opslužuje pristigle zahteve ukoliko takvi zahtevi postoje.
//
// Ako proces izvršava UI instrukciju on šalje zahtev UI podsistemu i odlazi u stanje čekanja radi završetka obrade UI zahteva. U čekanju proces otpušta procesor (tokom čekanja na UI obradu nema smisla da se drži procesor ako drugi procesi mogu da ga koriste) i tada bilo koji drugi proces iz liste spremnih procesa dobija procesor.
//
// UI instrukcija takođe prekida niz STANDARD instrukcija što znaci da se broj prethodno izvršenih STANDARD instrukcija za dati proces prilikom nailaska na UI instrukciju resetuje na 0.
//
// Kada UI podsistem obradi zahtev, proces koji je čekao obradu zahteva može da odmah postane aktivan (ukoliko je procesor slobodan) ili se uvezuje u listu spremnih procesa (ukoliko procesor koristi neki drugi proces).
//
// Operacija UI uređaja traje slučajan period između 100 i 300 ms.
//
// Implementirati označene metode, konstruktor i dodati privatne članove u klasu OS (fajl os.h).

#include <thread>
#include <vector>
#include <random>

#include "proces.hpp"
#include "dijagnostika.hpp"
#include "os.hpp"

using namespace std;

// Funkcija koju izvršavaju niti koje simuliraju procese:
void p(OS& s, int id) {
	default_random_engine dr;
	dr.seed(chrono::steady_clock::now().time_since_epoch().count());
	vector<INS_TYPE> instrukcije;
	discrete_distribution<unsigned> distribucija_instrukcija({9, 1});  // ova distribucija generise brojeve 0 ili 1; parametri su brojnost slucaja: 0 (STANDARD) sa verovatnocom 9/(9+1), 1 (UI) sa verovatnocom 1/(9+1)

	for (int i = 0; i < 10; i++) {
		instrukcije.push_back(static_cast<INS_TYPE>(distribucija_instrukcija(dr))); //9:1 šanse da će biti STANDARD instrukcija
	}
	Proces p(instrukcije, id);

	s.izvrsi_proces(p);

	s.getDijagnostika().proces_zavrsio(id);
}

// Funkcija koju izvršava nit koja simulira U/I podsistem:
void ui_podsistem(OS& s) {
	s.obradi_zahteve();
}

void testirajSve() {
    Dijagnostika d;
	OS OS(d, 3);
	thread procesi[5];

    // Kreiranje niti koja simulira U/I podsistem:
	thread ui_nit = thread(ui_podsistem, ref(OS));

    // Kreiranje niti koje simuliraju procese:
	for (int i = 0; i < 5; i++) {
		procesi[i] = thread(p, ref(OS), i+1);
	}

	for (thread &proces : procesi) {
		proces.join();
	}

	OS.zavrsi();
	ui_nit.join();  // posto telo ui_podsistem funkcije koristi metodu objekta OS kao svoju funkcionalnost, ne smemo dozvoliti da se desi destrukcija objekta a da nit ui_nit i dalje radi u pozadini!
}

int main() {
    testirajSve();

    return 0;
}
