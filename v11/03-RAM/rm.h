#ifndef RADNA_MEMORIJA_H_INCLUDED
#define RADNA_MEMORIJA_H_INCLUDED

#include "tip_procesa.h"
#include "dijagnostika.h"

using namespace std;
using namespace chrono;


class Radna_memorija {
private:
    	Dijagnostika& dijagnostika;

public:
        // parametar ukupno_okvira - ukupan broj okvira u radnoj memoriji
        Radna_memorija(Dijagnostika& d, int ukupno_okvira) : dijagnostika(d) {
            // proširiti po potrebi
        }

        Dijagnostika& getDijagnostika() {
            return dijagnostika;
        }
        // parametar broj_okvira - koliko okvira treba procesu za izvršavanje
        // parametar id_procesa  - jedinstveni identifikator procesa
        // parametar tip_procesa - da li je proces USER ili SYSTEM
        // nakon što pošalje zahtev, proces mora da sačeka dok opslužilac ne zauzme zahtevani broj okvira
        // i tada je potrebno pozvati metodu proces_ceka
        void napravi_zahtev(int broj_okvira, int id_procesa, Tip_procesa::tip_procesa t) {
           // Implementirati
        }

        // parametar id_procesa  - jedinstveni identifikator procesa

        void oslobodi(int id_procesa) {
            // Implementirati
        }

        // potrebno pozvati metodu ispisi_okvire nakon što proces zauzme okvire
        // potrebno pozvati metodu proces_zauzeo proces zauzme okvire
        void opsluzi() {
            // Implementirati
        }
};



#endif // RADNA_MEMORIJA_H_INCLUDED
