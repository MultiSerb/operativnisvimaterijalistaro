#include <iostream>
#include <thread>
#include <random>
#include "rm.h"
#include "tip_procesa.h"

using namespace std;
using namespace chrono;

void proces(Radna_memorija& ram, int id, int broj_stranica_procesa, Tip_procesa::tip_procesa tip) {
	ram.getDijagnostika().proces_pokrenut(id,broj_stranica_procesa,tip);

	ram.napravi_zahtev(broj_stranica_procesa, id, tip);
    this_thread::sleep_for(seconds(broj_stranica_procesa));
	ram.oslobodi(id);

    ram.getDijagnostika().proces_oslobodio(id,tip);
}

void opsluzilac(Radna_memorija& mem){
	while(true){
		mem.opsluzi();
	}
}

const int BROJ_OKVIRA = 5;
const int UKUPNO_PROCESA = 6;

void testirajSve() {
  default_random_engine dr(chrono::steady_clock::now().time_since_epoch().count());
  uniform_int_distribution<int> raspodela_br_stranica_procesa(1, 4);
  Dijagnostika d;
  Radna_memorija rm(d,BROJ_OKVIRA);

  thread t_ops = thread(opsluzilac, ref(rm));
  t_ops.detach();

  thread t_proc[UKUPNO_PROCESA];

  for (int i = 0; i < UKUPNO_PROCESA; ++i){
      // naizmenicno se prave po USER i SYSTEM procesi
      t_proc[i] = thread(proces, ref(rm), i+1, raspodela_br_stranica_procesa(dr), static_cast<Tip_procesa::tip_procesa>(i % 2));
   }

   for (int i = 0; i < UKUPNO_PROCESA; ++i)
		t_proc[i].join();
}

int main() {
    testirajSve();

    return 0;
}
