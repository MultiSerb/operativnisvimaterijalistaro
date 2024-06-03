#ifndef OS_H_INCLUDED
#define OS_H_INCLUDED

#include <list>
#include <mutex>
#include <condition_variable>

#include "dijagnostika.h"

using namespace std;
using namespace chrono;

struct UI_zahtev {			// Struktura UI zahteva.
	condition_variable cv;
	int broj_staze;
	bool obradjen;

	UI_zahtev(int b): broj_staze(b), obradjen(false) {
	}
};

class OS {
private:
    mutex m;
    Dijagnostika& dijagnostika;
   	list<shared_ptr<UI_zahtev>> zahtevi;    // lista POKAZIVAČA na UI zahteve. Neophodno je ovako jer CV unutar zahteva ne moze da se kopira.
	condition_variable ui;       // čekanje UI uređaja da neki proces zatraži U/I operaciju
	int trenutna_poz;			//  mora se pamtiti trenutna pozicija glave diska
    bool smer_obrade;
    int flag;

    list<shared_ptr<UI_zahtev>>::iterator nadji_sledeci() {
        auto najblizi = zahtevi.end();
		int razlika_najblize_poz, razlika_trenutne_poz;

        for (auto it = zahtevi.begin(); it != zahtevi.end(); it++) {
			if (smer_obrade) {
				razlika_trenutne_poz = (*it)->broj_staze - trenutna_poz;
			} else {
				razlika_trenutne_poz = trenutna_poz - (*it)->broj_staze;
            }
            if (razlika_trenutne_poz >= 0) {
                if (najblizi == zahtevi.end()) {
                    najblizi = it;
                    razlika_najblize_poz = razlika_trenutne_poz;
                } else if (razlika_trenutne_poz < razlika_najblize_poz) {
                    najblizi = it;
                    razlika_najblize_poz = razlika_trenutne_poz;
                }
            }
        }
        return najblizi;
    }
public:
    OS(Dijagnostika& d, int t, bool kvs) : dijagnostika(d) {
        trenutna_poz = t;
        smer_obrade = kvs;
		flag = -1;
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
    // Prema uslovima zadatka, metoda treba da obradi najblizi zahtev koji je u smeru kretanja glave
    //
    // Povratna vrednost metode treba da bude broj staze koji je obrađen.
    int obradi_zahtev() {
        unique_lock<mutex> l(m);
        while (zahtevi.empty())			//dogod nema zahteva ceka se da se neki stvori
            ui.wait(l);

        //nadji zahtev najblizi trenutnoj poziciji u smeru obrade
        auto sledeci = nadji_sledeci();

        l.unlock();
        this_thread::sleep_for(milliseconds(300));		//obrada zahteva
        l.lock();

        if (sledeci == zahtevi.end()) {
            if (smer_obrade) {
                if (trenutna_poz < 199)
                    trenutna_poz = 199;
                else
                    trenutna_poz = 0;
            } else {
                if (trenutna_poz > 0)
                    trenutna_poz = 0;
                else
                    trenutna_poz = 199;
            }
        } else {
            trenutna_poz = (*sledeci)->broj_staze; //disk se pomerio na novu poziciju i obradio zahtev

            (*sledeci)->obradjen = true;			//javi najblizam zahtevu da je obradjen i izbrisi ga iz list-a
            (*sledeci)->cv.notify_one();
            zahtevi.erase(sledeci);
        }

        return trenutna_poz;
    }
};

#endif // OS_H_INCLUDED

