#ifndef PROGRAM_HPP_INCLUDED
#define PROGRAM_HPP_INCLUDED

#include <string>
#include <mutex>
#include <condition_variable>
#include <queue>

#include "dijagnostika.hpp"
#include "naredba.hpp"
#include "povratna_vrednost.hpp"

using namespace std;
using namespace chrono;

struct Zahtev {
    public:
        int zauzmi;             // Koliko VM je potrebno zauzeti
        condition_variable cv;

        Zahtev(int zauzmi_memorije) {
            zauzmi = zauzmi_memorije;
        }

        void cekaj(unique_lock<mutex>& l) {
            while(zauzmi)
                cv.wait(l);
        }

        void obradjen() {
            zauzmi = 0;
            cv.notify_one();
        }
};

class Program {
private:
    Dijagnostika& dijagnostika;

    int slobodno_ram;               // Količina slobodnog RAM-a
    int zauzeto_virtuelna;          // Količina zauzete virtuelne memorije. Ova simulacija smatra da je VM neograničenog kapaciteta.

    mutex m;
    condition_variable virtuelna;   // Signal kojim se OS obaveštava da je potrebno da zauzme VM.

    bool zavrsen;                   // Indikator zavrsetka programa

    queue<shared_ptr<Zahtev>> zahtevi;         // Red zahteva za zauzimanje virtuelne memorije

    // Pomoćna metoda koja izvršava alokaciju memorije
    //
    // kolicina_memorije - koliko memorije je potrebno zauzeti
    void izvrsi_malloc(int kolicina_memorije) {
        unique_lock<mutex> l(m);
        if (slobodno_ram < kolicina_memorije) {         // Ukoliko ima manje RAM-a nego što je zahtevano

            virtuelna.notify_one();                     // notifikuje se CV. Aktivira se zauzimanje VM.
            
            shared_ptr<Zahtev> z = make_shared<Zahtev>(kolicina_memorije);  // Pravi se novi zahtev
            zahtevi.push(z);                            // Zahtev se dodaje u red zahteva
            z->cekaj(l);                                // Čekanje da se zahtev obradi

        } else
            slobodno_ram -= kolicina_memorije;      // Ako ima dovoljno RAM-a, on se zauzima

        this_thread::sleep_for(seconds(1));         //sleep (ako ima višestruki malloc). Simulira se da zauzimanje memorije traje neko vreme
    }

public:
    Program(Dijagnostika& d, int kapacitet) : dijagnostika(d) {
        slobodno_ram = kapacitet;
        zauzeto_virtuelna = 0;
        zavrsen = false;
    }

    Dijagnostika& getDijagnostika() {
        return dijagnostika;
    }

    // void zavrsi() {
    //     zavrsen = true;
    //     virtuelna.notify_one();
    // }

    // Metoda koju poziva nit koja simulira izvršenje programa, kako bi zauzela potrebnu memoriju
    //
    // naredba - naredba koja se izvršava (naredba kojom se zauzima memorija)
    // Vraća instancu klase Povratna_vrednost koja sadrži opis zauzeća memorije NAKON izvršenja naredbe.
    Povratna_vrednost izvrsi_naredbu(Naredba naredba) {
        if (naredba.tip == "repeat") {                          // Ukoliko je tip naredbe repeat.
            for (int i = 0; i < naredba.ponavljanja; ++i)       // operacija malloc se ponavlja zadat broj puta
                izvrsi_malloc(naredba.kolicina_memorije);
        } else {
            izvrsi_malloc(naredba.kolicina_memorije);           // U suprotnom, jednom se izvrši malloc.
        }
        return {slobodno_ram, zauzeto_virtuelna};               // Nakon zauzimanja memorije vraća se koliko ima slobodnog RAM-a i
                                                                // koliko je zauzeto virtuelne memorije.
    }

    // Metoda koju poziva nit koja simulira deo operativnog sistema koji se bavi zauzimanjem virtuelne memorije kako bi se zauzela određena količina VM
    // Potrebnu količinu VM (kao i bilo koju drugu neophodnu informaciju) preneti u nekom izabranom polju klase.
    // Potrebno je pozvati metodu dijagnostika.izvrseno_zauzimanje_vm nakon što je određena količina VM zauzeta.
    int zauzmi_virtuelnu_memoriju() {

        unique_lock<mutex> l(m);

        while (zahtevi.empty()) {  // && !zavrsen) {       // Dokle god nema aktiviranja OS i niti koje zauzimaju memoriju nisu završene, čeka se komanda
            virtuelna.wait(l);
        }

        // while (zavrsen) {
        //     this_thread::yield();                   // Procesor se prepušta nekoj od spremnih niti
        //                                             // Ovo se radi da uslovna promenljiva ne bi ostala u čekanju
        //                                             // Zato što bi to blokiralo uništavanje objekta i onemogućilo program da se završi
        // }

        this_thread::sleep_for(seconds(1));         // Zauzimanje virtuelne memorije traje 1 sekund.

        shared_ptr<Zahtev> z = zahtevi.front();                // Preuzmi zahtev sa vrha reda zahteva
        zauzeto_virtuelna += z->zauzmi;             // Povećaj količinu zauzete virtuelne memorije
        z->obradjen();                              // Označi zahtev kao obrađen
        zahtevi.pop();                              // Ukloni obrađeni zahtev iz reda zahteva

        return zauzeto_virtuelna;
    }
};

#endif // PROGRAM_HPP_INCLUDED
