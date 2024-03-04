/**
 * Napisati C++ program koji iz ulazne datoteke cita podatke o temperaturi u toku vikenda sa ski 
 * staza na Kopaoniku, Zlatiboru i Jahorini i odredjuje koji dani i na kojoj planini su idealni 
 * za skijanje a koji nisu. Neki idealni opseg temperature za skijanje je od -5 do 3 stepena.
 * 
 * Za svaku od planina postoji posebna datoteka cije ime se sastoji od imena planine i prosirenja
 * (ekstenzije) ".txt". U svakoj pojedinacnoj datoteci se u jednom redu nalaze podaci za jedan dan. 
 * Jedan red sadrzi redom ime_dana, datum, i potom izmerene temperature. Temperatura se meri na 
 * svakih sat vremena, pocevsi od 8 ujutru, do 8 uvece. Svi podaci su odvojeni razmakom.
 * 
 * Izgled jednog reda iz ulaznih datoteka "Kopaonik.txt" "Zlatibor.txt" "Jahorina.txt"
 * 
 *     subota 01.02.  -15 -13 -10 -8 -3 0 -2 -3 2 2 -5 -7 -3
 * 
 * Treba za svaki dan pronaci najnizu i najvisu dnevnu temperaturu. Ukoliko minimalna i maksimalna
 * temperatura upadaju u navedeni opseg, treba informacije za taj dan ispisati u datoteku 
 * "idealno.txt", u suprotnom u datoteku "lose.txt".
 *
 * Ispis u izlaznu datoteku treba da prati format:
 *     <ime_planine> [<ime_dana> <datum>] <min. temp.> >> <maks. temp.>
 * 
 * Primer ispisa u bilo kojoj od izlaznih datoteka "idealno.txt", "lose.txt":
 * 
 *     Kopaonik [subota 01.02.] -15 >> 2
 *
 * Treba napraviti jednu nit koja ce samo citati podatke iz ulaznih datoteka, jednu nit koja ce 
 * samo pisati spremljene podatke u izlazne datoteke i 4 niti radnika koji ce na osnovu podataka iz
 * ulaznih datoteka generisati sve neophodno za ispis u izlazne datoteke.
*/
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <fstream>
#include <sstream>

#define BR_RADNIKA 4
#define BR_MERENJA_TEMPERATURE 13
#define MINIMALNA_IDEALNA_TEMPERATURA -5
#define MAKSIMALNA_IDEALNA_TEMPERATURA 3

using namespace std;

class StanjeSkijalistaNaDan {
private:
    string planina;
    string dan;
    string datum;
    vector<float> temperature;
    float minimum, maksimum;
public:
    StanjeSkijalistaNaDan(): minimum(0), maksimum(0) {
        temperature.reserve(BR_MERENJA_TEMPERATURE);
    }
    void postaviPlaninu(string p) {
        planina = p;
    }
    void postaviDan(string d) {
        dan = d;
    }
    void postaviDatum(string d) {
        datum = d;
    }
    void dodajTemperaturu(float t) {
        temperature.push_back(t);
        azurirajMinMaks();
    }
    float dobaviMinTemperaturu() {
        return minimum;
    }
    float dobaviMaksTemperaturu() {
        return maksimum;
    }

    bool daLiJeIdealanOpsegTemperatura() {
        return (minimum >= MINIMALNA_IDEALNA_TEMPERATURA) && (maksimum <= MAKSIMALNA_IDEALNA_TEMPERATURA);
    }

    friend ostream& operator<<(ostream &os, StanjeSkijalistaNaDan ss) {
        return os << ss.planina << " " << "[" << ss.dan << " " << ss.datum << "] " << ss.minimum << " >> " << ss.maksimum << endl;
    }

private:
    void azurirajMinMaks() {
        if (temperature.size() <= 0)
            return;
        else if (temperature.size() == 1) {
            minimum = maksimum = temperature[0];
        } else {
            float poslednja_temperatura = temperature.back();
            if (minimum > poslednja_temperatura) {
                minimum = poslednja_temperatura;
            }
            if (maksimum < poslednja_temperatura) {
                maksimum = poslednja_temperatura;
            }
        }
    }
};

/** Klasa koja modeluje "postansko sanduce" izmedju citaca i radnika.
*/
template<typename T>
class RedoviIzDatoteke {
private:
    mutex podaci_mx;                       // propusnica za sinhronizaciju nad svim poljima klase
    queue<T> redovi;
    condition_variable podaci_spremni;
    bool kraj;
public:
    RedoviIzDatoteke(): kraj(false) {}  // na pocetku nije kraj i nema radnika

    void dodaj(T redIzDatoteke) {
        unique_lock<mutex> propusnica(podaci_mx);
        redovi.push(redIzDatoteke);
    }

    bool preuzmi(T &redIzDatoteke) {
        unique_lock<mutex> propusnica(podaci_mx);
        while (daLiCekamPodatke()) {
            podaci_spremni.wait(propusnica);
        }

        if (jeLiKraj()) {
            return false;  // signal pozivajucem okruzenju da nece vise biti podataka
        }

        redIzDatoteke = redovi.front();
        redovi.pop();
        return true;  // signal pozivajucem okruzenju da je podataka uspesno preuzet iz bafera
    }

    // Imamo jednu nit koja ce stvarati podatke u ovom baferu podataka, ona zna kada je kraj i dovoljno je da ona jednim pozivom ove metode objavi kraj za sve ostale
    void objaviKraj() {
        unique_lock<mutex> propusnica(podaci_mx);
        kraj = true;
        podaci_spremni.notify_all();  // za slucaj da postoje radnici koji cekaju na nove podatke, budjenje istih da ne cekaju ako nema vise podataka da se preuzme jer je kraj
    }

private:
    /**
     * Provera da li treba da cekamo podatke. Vraca istinu samo onda kada u kolekciji
     * nema podataka ali istovremeno i nije objavljen kraj stvaranja podataka.
    */
    bool daLiCekamPodatke() {
        return redovi.empty() && !kraj;
    }

    /**
     * Provera da li smo zavrsili sa citanjem podataka. Vraca istinu samo onda kada nema vise podataka
     * u kolekciji i sve niti stvaraoci podataka su se odjavili.
    */
    bool jeLiKraj() {
        return redovi.empty() && kraj;
    }
};


/** Klasa koja modeluje "postansko sanduce" izmedju radnika i pisaca.
*/
template<typename T>
class PodaciZaIspis {
private:
    mutex podaci_mx;                       // propusnica za sinhronizaciju nad svim poljima klase
    queue<T> stanjaSkijalista;
    condition_variable podaci_spremni;
    bool kraj;
    int br_stvaralaca_podataka;
public:
    PodaciZaIspis(): kraj(false), br_stvaralaca_podataka(0) {}  // na pocetku nije kraj i nema radnika

    void dodaj(T stanjeSkijalista) {
        unique_lock<mutex> propusnica(podaci_mx);
        stanjaSkijalista.push(stanjeSkijalista);
    }

    bool preuzmi(T &stanjeSkijalista) {
        unique_lock<mutex> propusnica(podaci_mx);
        while (daLiCekamPodatke()) {
            podaci_spremni.wait(propusnica);
        }

        if (jeLiKraj()) {
            return false;
        }

        stanjeSkijalista = stanjaSkijalista.front();
        stanjaSkijalista.pop();
        return true;
    }

    /**
     * Ovu metodu pre pocetka svog rada treba da pozovu stvaraoci podataka ovog bafera.
    */
    void prijavaStvaraocaPodataka() {
        unique_lock<mutex> propusnica(podaci_mx);
        br_stvaralaca_podataka++;
    }

    /**
     * Ovu metodu na kraju svog rada treba da pozovu stvaraoci podataka ovog bafera.
    */
    void odjavaStvaraocaPodataka() {
        unique_lock<mutex> propusnica(podaci_mx);
        br_stvaralaca_podataka--;
        if (!br_stvaralaca_podataka) {  // kada se desi odjava, proveravamo da li je broj stvaralaca pao na nula; ako jeste, znaci da nece biti novih podataka vise te treba objaviti kraj i probuditi uspavane niti
            kraj = true;
            podaci_spremni.notify_one();  // na izlazu moze da ceka samo nit koja pise u datoteku (u ovom konkretnom slucaju), pa je ovde dovoljan notify_ONE()
        }
    }

private:
    /**
     * Provera da li treba da cekamo podatke. Vraca istinu samo onda kada u kolekciji
     * nema podataka ali istovremeno i nije objavljen kraj stvaranja podataka.
    */
    bool daLiCekamPodatke() {
        return stanjaSkijalista.empty() && !kraj;
    }

    /**
     * Provera da li smo zavrsili sa citanjem podataka. Vraca istinu samo onda kada nema vise podataka
     * u kolekciji i sve niti stvaraoci podataka su se odjavili.
    */
    bool jeLiKraj() {
        return stanjaSkijalista.empty() && kraj;
    }
};

void citac(vector<string> imena_ulaznih_datoteka, RedoviIzDatoteke<string>& redovi_iz_ulaznih_datoteka) {
    for (string ime_ulazne_dat : imena_ulaznih_datoteka) {
        ifstream ulazna_dat(ime_ulazne_dat);
        cout << "CITAC: Obradjujem datoteku \"" << ime_ulazne_dat << "\"." << endl;

        if(ulazna_dat.is_open()) {
            string planina = ime_ulazne_dat.substr(0, ime_ulazne_dat.find(".txt"));
            string red_datoteke;
            while(getline(ulazna_dat, red_datoteke)) {  // citanje datoteke red po red
                redovi_iz_ulaznih_datoteka.dodaj("" + planina + " " + red_datoteke); 
            }

            ulazna_dat.close();
        } else
            cerr << "CITAC: Nisam mogao da otvorim datoteku \"" << ime_ulazne_dat << "\" za citanje!\n";
    }

    redovi_iz_ulaznih_datoteka.objaviKraj();  // citac je jedna jedina nit koja stvara podatke tako da je dovoljno da samo ona kada zavrsi posao objavi kraj
}

void radnik(RedoviIzDatoteke<string>& redovi_iz_ulaznih_datoteka, PodaciZaIspis<StanjeSkijalistaNaDan>& pripremljeni_podaci){
    pripremljeni_podaci.prijavaStvaraocaPodataka();  // radnik stvara podatke za izlazni bafer tako da se i prijavljuje kod njega kao stvaralac podataka

    string red_iz_datoteke;
    while (redovi_iz_ulaznih_datoteka.preuzmi(red_iz_datoteke)) {
        string rec;
        float temperatura;
        StanjeSkijalistaNaDan stanjeSk;
        istringstream red_iz_datoteke_tok(red_iz_datoteke);

        if (red_iz_datoteke_tok >> rec) {
            stanjeSk.postaviPlaninu(rec);
        } else 
            continue;

        if (red_iz_datoteke_tok >> rec) {
            stanjeSk.postaviDan(rec);
        } else 
            continue;

        if (red_iz_datoteke_tok >> rec) {
            stanjeSk.postaviDatum(rec);
        } else 
            continue;
        
        bool greska_prilikom_citanja = false;
        for (int i = 0; i < 13; i++) {
            if (red_iz_datoteke_tok >> temperatura) {
                stanjeSk.dodajTemperaturu(temperatura);
            } else {
                greska_prilikom_citanja = true;
                break;
            }
        }
        if (greska_prilikom_citanja)
            continue;

        pripremljeni_podaci.dodaj(stanjeSk);
    }

    pripremljeni_podaci.odjavaStvaraocaPodataka();  // radnik se odjavljuje; poslednji radnik koji se odjavi uzrokuje objavu kraja u izlaznom baferu
}

void pisac(PodaciZaIspis<StanjeSkijalistaNaDan>& pripremljeni_podaci) {
    ofstream idealno_dat("idealno.txt");
    ofstream lose_dat;
    StanjeSkijalistaNaDan stanjeSk;

    if (idealno_dat.is_open()) {
        lose_dat.open("lose.txt");
        if (!lose_dat.is_open()) {
		    cerr << "PISAC: Nisam mogao da otvorim datoteku \"lose.txt\" za pisanje! Zatvaram vec otvorenu \"idealno.txt\" datoteku i prekidam dalji rad.\n";
            idealno_dat.close();
            return;
        }

        while (pripremljeni_podaci.preuzmi(stanjeSk)) {
            if (stanjeSk.daLiJeIdealanOpsegTemperatura()) {
                idealno_dat << stanjeSk;
            } else {
                lose_dat << stanjeSk;
            }
        }

        lose_dat.close();
        idealno_dat.close();
    } else
		cerr << "PISAC: Nisam mogao da otvorim datoteku \"idealno.txt\" za pisanje!\n";
}


int main() {
    RedoviIzDatoteke<string> redovi_iz_ulaznih_datoteka;
    PodaciZaIspis<StanjeSkijalistaNaDan> pripremljeni_podaci;
    vector<string> imena_ulaznih_datoteka = {"Kopaonik.txt", "Zlatibor.txt", "Jahorina.txt"};

    thread th_reader(citac, imena_ulaznih_datoteka, ref(redovi_iz_ulaznih_datoteka));
    thread th_writer(pisac, ref(pripremljeni_podaci));
    thread th_workers[BR_RADNIKA];

    for(auto &th: th_workers){
        th = thread(radnik, ref(redovi_iz_ulaznih_datoteka), ref(pripremljeni_podaci));
    }

    th_reader.join();
    for(auto &th: th_workers) {
        th.join();
    }
    th_writer.join();
    
    return 0;
}
