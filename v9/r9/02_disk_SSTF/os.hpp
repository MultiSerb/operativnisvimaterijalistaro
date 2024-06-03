#ifndef OS_HPP_INCLUDED
#define OS_HPP_INCLUDED

#include <deque>
#include <mutex>
#include <condition_variable>

#include "dijagnostika.hpp"

using namespace std;
using namespace chrono;

struct UI_zahtev {			// Struktura UI zahteva. Istovetna kao kod zadatka 02-procesi u vezbama 09 procesi.
	condition_variable cv;
	int broj_staze;
	bool obradjen;

	UI_zahtev(int b): broj_staze(b), obradjen(false) {
	}
};

class OS {
private:
    Dijagnostika& dijagnostika;
    mutex m;
   	deque<shared_ptr<UI_zahtev>> zahtevi;   // red POKAZIVAČA na UI zahteve. Neophodno je ovako jer CV unutar zahteva ne moze da se kopira.
	condition_variable ui;       // čekanje UI uređaja da neki proces zatraži U/I operaciju
	int trenutna_poz;			//  mora se pamtiti trenutna pozicija glave diska
    bool kraj;
    milliseconds ukupno_trajanje;
public:
    OS(Dijagnostika& d, int t) : dijagnostika(d), ukupno_trajanje(0) {
        trenutna_poz = t;
        kraj = false;
    }

    Dijagnostika& getDijagnostika() {
        return dijagnostika;
    }

    // Metoda koju poziva nit koja simulira izvršenje procesa, kako bi se uputio zahtev ka U/I uređaju
    //
    // id_procesa - ID procesa koji upućuje zahtev
    // broj_staze - broj staze diska kojoj nit želi da pristupi
    //
    // Metoda treba da formira novi zahtev i uputi ga U/I uređaju na obradu. Proces će biti blokiran dok god se ovaj zahtev ne izvrši. Pre nego što stupi u blokadu, potrebno je pozvati dijagnostika.proces_ceka.
    void uputi_UI_zahtev(int id_procesa, int broj_staze) {
        unique_lock<mutex> l(m);
        shared_ptr<UI_zahtev> z = shared_ptr<UI_zahtev>(new UI_zahtev(broj_staze));
        zahtevi.push_back(z);
        ui.notify_one();

        while (!z->obradjen) {
            dijagnostika.proces_ceka(id_procesa, broj_staze);
            z->cv.wait(l);
        }
    }

    // Metoda koju poziva nit koja simulira deo operativnog sistema koji upravlja diskom, kako bi se obradio jedan pristigli zahtev
    //
    // Prema uslovima zadatka, metoda treba da obradi zahtev kojim se pristupa onoj stazi koja je najbliža stazi na kojoj je trenutno glava diska
    //
    // Povratna vrednost metode treba da bude broj staze koji je obrađen.
    int obradi_zahtev() {
        unique_lock<mutex> l(m);
        while (zahtevi.empty() && !kraj)			//dogod nema zahteva ceka se da se neki stvori
            ui.wait(l);

        if (kraj) return -1;

        //nadji zahtev najblizi trenutnoj poziciji
        deque<shared_ptr<UI_zahtev>>::iterator najblizi = zahtevi.begin();
        for (auto it = zahtevi.begin(); it != zahtevi.end(); it++) {
            if ( abs(trenutna_poz - ((*it)->broj_staze)) < abs(trenutna_poz - (*najblizi)->broj_staze)) {
                najblizi = it;
            }
        }

        milliseconds trajanje_obrade(abs(trenutna_poz - (*najblizi)->broj_staze)*10);  // svaki prelazak sa staze na stazu je 10ms

        l.unlock();
        this_thread::sleep_for(trajanje_obrade);		//obrada zahteva
        l.lock();

        trenutna_poz = (*najblizi)->broj_staze; //disk se pomerio na novu poziciju i obradio zahtev
        ukupno_trajanje += trajanje_obrade;
        
        (*najblizi)->obradjen = true;			//javi najblizam zahtevu da je obradjen i izbrisi ga iz deque-a
        (*najblizi)->cv.notify_one();
        int ret_val = (*najblizi)->broj_staze;
        zahtevi.erase(najblizi);

        return ret_val;
    }

    // Metoda koja vraca sumu svih vremena potrosenih za obradu zahteva
    int dobaviUkupnoTrajanje() {
        unique_lock<mutex> l(m);
        return ukupno_trajanje.count();
    }

    // Metoda koja postavlja signal za kraj izvršavanja i pokreće obrađivač iz čekanja kako bi program mogao da se završi
    void zavrsi() {
        unique_lock<mutex> l(m);
        kraj = true;
        ui.notify_one();
    }
};

#endif // OS_HPP_INCLUDED
