#ifndef RADNA_MEMORIJA_H_INCLUDED
#define RADNA_MEMORIJA_H_INCLUDED

#include "tip_procesa.h"
#include "dijagnostika.h"

#include <queue>
#include <vector>
#include <condition_variable>
#include <mutex>


using namespace std;
using namespace chrono;

struct Zahtev {	//struktura zahteva
		int br_okvira;
		int id_procesa;
		Tip_procesa::tip_procesa tip;
		bool zavrsen;
		condition_variable cv;
		Zahtev(int br_ok, int id, Tip_procesa::tip_procesa t):
		    br_okvira(br_ok), id_procesa(id), tip(t), zavrsen(false) {}
};

class Radna_memorija {
private:
    	mutex m;
    	Dijagnostika& dijagnostika;
        vector<int> okviri; //Evidencija svih okvira u memoriji.
                            //Svaki zauzeti okvir se evidentira id-em procesa koji ga je zauzeo.
                            //Slobodni okviri se evidentiraju sa -1
		int slobodnih_okvira; //broj slobodnih okvira

		condition_variable cv_opsluzilac;   //signal opslužiocu da krene da radi
		condition_variable oslobodjena_memorija;  //signal opslužiocu da je memorija oslobođena
		queue<shared_ptr<Zahtev>> sys_zahtevi;    // red SYSTEM zahteva - FIFO princip
		vector<shared_ptr<Zahtev>> user_zahtevi;  // vektor USER zahteva - princip najmanji broj okvira

public:
        // parametar ukupno_okvira - ukupan broj okvira u radnoj memoriji
        Radna_memorija(Dijagnostika& d, int ukupno_okvira) : dijagnostika(d), slobodnih_okvira(ukupno_okvira) { //ukupan broj okvira u memoriji
			okviri.resize(ukupno_okvira, -1);
		}

        Dijagnostika& getDijagnostika() {
            return dijagnostika;
        }
        // parametar broj_okvira - koliko okvira treba procesu za izvršavanje
        // parametar id_procesa  - jedinstveni identifikator procesa
        // parametar tip_procesa - da li je proces USER ili SYSTEM
        void napravi_zahtev(int broj_okvira, int id_procesa, Tip_procesa::tip_procesa t) {
            unique_lock<mutex> l(m);
			shared_ptr<Zahtev> z = shared_ptr<Zahtev>(new Zahtev(broj_okvira, id_procesa, t)); //stvaranje novog zahteva

            if(t == Tip_procesa::SYSTEM) //u zavisnosti od tipa zahteva stavi ga u odgovarajući red
                sys_zahtevi.push(z);
            else
                user_zahtevi.push_back(z);

			cv_opsluzilac.notify_one();		//javi opslužiocu da proces traži memoriju

			while (!z->zavrsen)	{
				dijagnostika.proces_ceka(id_procesa,broj_okvira,t);
				z->cv.wait(l);
			}
    }

    // parametar id_procesa  - jedinstveni identifikator procesa
    void oslobodi(int id_procesa) {

        for (auto it = okviri.begin(); it != okviri.end(); ++it) {
				if (*it == id_procesa) {
					*it = -1;
					slobodnih_okvira++;
				}
        }
        // notifikacija opslužioca da je memorija oslobođena i da je dodeli nekom drugome
        oslobodjena_memorija.notify_one();

    }

    void opsluzi() {
            unique_lock<mutex> l(m);

			while(sys_zahtevi.empty() && user_zahtevi.empty()) {  //ako nema zahteva ni jednog prioriteta opslužioc čeka
				cv_opsluzilac.wait(l);
			}

			shared_ptr<Zahtev> z;
            vector<shared_ptr<Zahtev>>::iterator najmanji = user_zahtevi.begin();

            //prolazi kroz liste zahteva po prioritetima i uzima najprioritetniji
			if (!sys_zahtevi.empty()){
                z = sys_zahtevi.front();
                sys_zahtevi.pop();

			} else {

                for(auto it= user_zahtevi.begin(); it!=user_zahtevi.end();it++) {
                    if((*najmanji)->br_okvira > (*it)->br_okvira) {
                        najmanji = it;
                        break;
                    }
                }

                z = *najmanji;
                user_zahtevi.erase(najmanji);
			}

			// ukoliko nema dovoljno slobodnih okvira, za dati zahtev čekati dok se memorija ne oslobodi
			while (slobodnih_okvira < z->br_okvira) {
				oslobodjena_memorija.wait(l);
			}

			 // Ukoliko ima dovoljno okvira, zauzmi ih za dati proces iz zahteva
			int proces_zauzeo_okvira = 0;
			for (auto it = okviri.begin(); it != okviri.end(); it++) {
				if (*it == -1)  {
					*it = z->id_procesa;
					slobodnih_okvira--;
					if (++proces_zauzeo_okvira == z->br_okvira)  //zauzeto okvira koliko procesu treba
						break;
				}
			}
            dijagnostika.proces_zauzeo(z->id_procesa,z->br_okvira,z->tip);
            dijagnostika.ispisi_okvire(okviri.begin(), okviri.end());
			z->zavrsen = true; //zahtev ispunjen, proces se budi iz čekanja
			z->cv.notify_one();
    }
};



#endif // RADNA_MEMORIJA_H_INCLUDED
