#ifndef OS_HPP_INCLUDED
#define OS_HPP_INCLUDED

#include <queue>
#include <mutex>
#include <condition_variable>
#include <random>

#include "dijagnostika.hpp"
#include "proces.hpp"

using namespace std;
using namespace chrono;

// Struktura UI zahteva. Neophodna radi efikasne obrade zahteva. Moguće su i drugačije implementacije.
// Npr. odvojeni redovi čekanja, integera i bool-ova ali je efikasnije spakovati sve u jednu strukturu i onda date strukture stavljati u red (queue) (preko pokazivača, dinamički alocirane)
struct UI_zahtev {
	condition_variable cv;
	int id_procesa;
	bool obradjen;
	UI_zahtev(int i): id_procesa(i), obradjen(false) {}
};

class OS {
private:
    Dijagnostika& dijagnostika;
    mutex m;
    int maks_uzastopnih_instrukcija;
    int aktivni_proces;
    default_random_engine dr;
    uniform_int_distribution<int> trajanje_standard_instrukcije, trajanje_ui_obrade;
    queue<shared_ptr<UI_zahtev>> red_UI_zahteva; // Red POKAZIVAČA na UI zahteve. Neophodno je ovako jer CV unutar zahteva ne može da se kopira.
    condition_variable spremni; // Red spremnih procesa
    condition_variable ui; // Čekanje ui uređaja da neko zatraži operaciju
    bool gotovo; // Indikator kraja obrade zahteva

public:
    // d                 - Referenca na objekat Dijagnostika, koji služi za ispis stanja simulacije
    // maks_uzastopnih_i - Maksimalan dozvoljen broj uzastopnih STANDARD instrukcija, nakon kojeg obavezno sledi preključivanje
    OS(Dijagnostika& d, int maks_uzastopnih_i) : dijagnostika(d) {
        maks_uzastopnih_instrukcija = maks_uzastopnih_i;
        aktivni_proces = -1;
        gotovo = false;
        dr.seed(steady_clock::now().time_since_epoch().count());
        trajanje_standard_instrukcije = uniform_int_distribution<>(10, 50);
        trajanje_ui_obrade = uniform_int_distribution<>(100, 300);
    }

    // Pomoćna metoda koja vraća objekat klase Dijagnostika koji služi za ispis stanja simulacije
    Dijagnostika& getDijagnostika() {
        return dijagnostika;
    }

    // Metoda koju poziva nit koja simulira proces kako bi se izvršile sve instrukcije procesa.
    //
    // p - Struktura koja sadrži opis procesa (ID procesa i instrukcije koje treba izvršiti)
    //
    // Ukoliko proces ne može da se izvrši (procesor je zauzet), proces mora da se uveže u red spremnih i treba pozvati metodu dijagnostika.proces_ceka.
    // Ukoliko dođe do toga da izvršenje procesa prestaje (dostignut je maksimalan dozvoljen broj uzastopnih STANDARD instrukcija ili su sve instrukcije procesa izvršene), potrebno je pozvati dijagnostika.proces_oslobadja_procesor.
    // Kada se izvrši STANDARD instrukcija nekog procesa, potrebno je pozvati dijagnostika.proces_izvrsio_standardnu.
    // Kada se izvršava UI instrukcija, potrebno je pozvati par metoda: dijagnostika.proces_ceka_na_UI kada se pokrene U/I operacija (nakon čega sledi čekanje na U/I podsistem) i dijagnostika.proces_zavrsio_UI kada se završi U/I operacija.
	void izvrsi_proces(Proces& p) {
        int uzastopnih_instrukcija = 0; // Broj uzastopno izvršenih instrukcija
        for (vector<INS_TYPE>::iterator it = p.instrukcije.begin(); it != p.instrukcije.end(); it++) {
            unique_lock<mutex> l(m);
            // Proces mora da sačeka ako je procesor trenutno dodeljen nekom drugom procesu:
            while (aktivni_proces != p.id && aktivni_proces != -1) {
                dijagnostika.proces_ceka(p.id);
                spremni.wait(l);
            }
            aktivni_proces = p.id; // Proces zauzima procesor

            if (*it == STANDARD) {
                // Simulacija trajanja izvršavanja standardne instrukcije:
                l.unlock();
                this_thread::sleep_for(milliseconds(trajanje_standard_instrukcije(dr)));
                l.lock();
                dijagnostika.proces_izvrsio_standardnu(p.id);
                // Ako je izvršen maksimalan broj uzastopnih instrukcija ili su izvršene sve instrukcije procesa:
                if (++uzastopnih_instrukcija == maks_uzastopnih_instrukcija || it == p.instrukcije.end()-1) {
                    dijagnostika.proces_oslobadja_procesor(p.id);
                    uzastopnih_instrukcija = 0;
                    aktivni_proces = -1;    // Oslobodi procesor
                    spremni.notify_one();   // Aktiviraj sledeći iz reda spremnih
                    // Cooldown period, da ne bi ovaj isti proces odmah ponovo preuzeo procesor:
                    l.unlock();
                    this_thread::sleep_for(milliseconds(100));
                }
            } else {
                // Formiramo UI zahtev i dodamo zahtev u red zahteva:
                // shared_ptr<UI_zahtev> zahtev = shared_ptr<UI_zahtev>(new UI_zahtev(p.id));  // ovo je jedna varijanta, ispod druga;
                shared_ptr<UI_zahtev> zahtev = make_shared<UI_zahtev>(p.id);
                // U dijagnosticke svrhe, otkomentarisati liniju ispod a zakomentarisati liniju iznad; 
                // shared_ptr<UI_zahtev> zahtev = shared_ptr<UI_zahtev>(new UI_zahtev(p.id), [](UI_zahtev *i)->void {cout << "brisem memoriju >> PID: " << i->id_procesa << endl; delete i;});
                // linija iznad dobija lambdu koja se poziva prilikom destrukcije POSLEDNJEG deljenog pokazivaca

                red_UI_zahteva.push(zahtev);
                ui.notify_one();

                dijagnostika.proces_ceka_na_UI(p.id);

                uzastopnih_instrukcija = 0;     // Resetuje se brojač uzastopnih instrukcija
                aktivni_proces = -1;            // Oslobađa se procesor
                spremni.notify_one();           // Javlja se sledećem spremnom da preuzme procesor

                while (!zahtev->obradjen) {
                    zahtev->cv.wait(l);                  // Čeka se na obradu zahteva
                }
                dijagnostika.proces_zavrsio_UI(p.id);
            }
        }
	}

    // Metoda koju poziva nit koja simulira obrađivač U/I zahteva kako bi se obradili trenutno pohranjeni U/I zahtevi.
    //
    // Potrebno je pozvati dijagnostika.ui_ceka kada ne postoje trenutno pohranjeni U/I zahtevi i obrađivač čeka na prvi.
    // Potrebno je pozvati dijagnostika.ui_zapocinje kada obrađivač krene u obradu U/I zahteva. Kada obrađivač završi taj zahtev, potrebno je pozvati dijagnostika.ui_zavrsio.
	void obradi_zahteve() {
        while (true) {
            unique_lock<mutex> l(m);
            while (red_UI_zahteva.empty() && !gotovo) {                // Obrada zahteva ne može da počne dok god nema ni jedan zahtev u redu
                dijagnostika.ui_ceka();
                ui.wait(l);
            }

            if (gotovo) return;

            shared_ptr<UI_zahtev> zahtev = red_UI_zahteva.front();          // Kada ima zahteva uzima se prvi iz reda (front)
            red_UI_zahteva.pop();                           // Odmah nakon toga se taj isti zahtev briše iz reda jer je uzet pre toga

            dijagnostika.ui_zapocinje(zahtev->id_procesa);

            l.unlock();
            this_thread::sleep_for(milliseconds(trajanje_ui_obrade(dr)));
            l.lock();

            zahtev->obradjen = true;                             // Postavljanje flag-a da je zahtev obrađen
            zahtev->cv.notify_one();                             // Javljanje niti koja čeka na obradu zahteva

            dijagnostika.ui_zavrsio(zahtev->id_procesa);
        }
	}

    // Signaliziranje kraja izvršavanja niti koja simulira obrađivač U/I zahteva
    void zavrsi() {
        unique_lock<mutex> l(m);
        gotovo = true;
        ui.notify_one();
    }
};

#endif // OS_HPP_INCLUDED
