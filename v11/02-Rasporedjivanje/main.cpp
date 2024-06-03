/* Napisati konkurentni program koji simulira proporcionalni raspoređivač u jednoprocesorskom sistemu.

Procesi su predstavljeni nitima. Da bi se izvršili pozivaju metodu "izvrsi" deljene promenljive tipa Rasporedjivac. Svaki proces sadrži određeni broj naredbi. Naredbe se izvršavaju jedna po jedna. Za svaku naredbu, proces mora da sačeka da procesor bude slobodan. Izvršavanje jedne naredbe traje 300 ms. Nakon izvršavanja naredbe, proces odlazi u stanje spreman, a drugi spreman proces dobija procesor.

Proporcionalni raspoređivač se zasniva na tome da je svakom procesu inicijalno dodeljen određeni broj vremenskih odsečaka. Npr. pretpostavimo da je ukupan broj vremenskih odsečaka u sistemu 100, i da procesi A, B i C dobijaju određeni broj odsečaka. Recimo da je A dobio 50 osečaka, B 30 a C 20 osečaka. Ovo znači da proces A treba da dobije 50% procesorskog vremena (50% šanse da sledeće preključivanje bude na njega, proces B 30% šanse i proces C 20% šanse).

Pri izboru sledeće naredbe (sledećeg procesa) za izvršavanje treba imati u vidu ovu raspodelu. Takođe kada jedan proces završi sve svoje naredbe treba obezbediti da se njegovi odsečci više ne računaju u vremensku raspodelu - npr. kada proces A završi ostaje samo 50 osečaka procesa B i C (u raspodeli 30-20).
*/

#include <vector>
#include <thread>
#include <random>

#include "rasporedjivac.h"

using namespace std;

void proces(Rasporedjivac& r, int broj_odsecaka, int id_procesa, int broj_naredbi) {
    Proces p(id_procesa, broj_naredbi, broj_odsecaka);
    
	r.getDijagnostika().proces_kreiran(id_procesa, broj_naredbi, broj_odsecaka);

    r.izvrsi(p);

    r.getDijagnostika().proces_zavrsio(id_procesa);
}

const int BROJ_PROCESA = 3;

void testirajSve() {
  default_random_engine dr(chrono::steady_clock::now().time_since_epoch().count());
  uniform_int_distribution<int> raspodela_br_naredbi(1, 10);

    Dijagnostika d;
	Rasporedjivac r(d);
	thread procesi[BROJ_PROCESA];

	vector<int> odsecci_procesa = {50, 30, 20};

	for (int i = 0; i < BROJ_PROCESA; i++) {
		procesi[i] = thread(proces, ref(r), odsecci_procesa[i], i+1, raspodela_br_naredbi(dr));
    }

	for (int i = 0; i < BROJ_PROCESA; i++)
		procesi[i].join();
}

int main() {
    testirajSve();

    return 0;
}
