/**
 * Napisati C++ program koji cita podatke o studentima iz ulazne datoteke i potom 
 * za svakog studenta racuna prosek
 * 
 * U ulaznoj datoteci "studenti.csv" se u svakom redu nalaze informacije o studentu: 
 *     Ime,Prezime,Broj indeksa,Ocene 
 * Pritom su ocene odvojene medjusobno razmacima.
 * 
 * Prilikom obrade podataka o studentima, mora se proveriti format indeksa da li je validan. Ako nije, zanemariti taj unos.
 * Format indeksa je:
 *     [[:alpha:]][[:alnum:]]{1,2}\s[[:digit:]]{1,3}\/[[:digit:]]{4}
 * 
 * U tri izlazne datoteke rasporediti studente u zavisnosti od proseka.
 * Ukoliko je prosek > 9.00 potrebno je upisati studenta u datoteku "kandidati_stipendija.csv".
 * Ukoliko je prosek > 8.00 i prosek <= 9.00 potrebno je upisati studenta u datoteku "kandidati_kredit.csv".
 * U ostalim slucajevima upisati studenta u datoteku "ostali.csv".
 * Format u izlaznoj datoteci treba da odgovara sledecem: Ime,Prezime,Broj_indeksa,prosek
 * 
 * Treba napraviti jednu nit koja ce samo citati redove ulazne podatke, jednu nit 
 * koja ce samo pisati gotove podatke u izlazne datoteke i 10 niti radnika koji ce na osnovu redova
 * iz ulazne datoteke generisati sve neophodno za ispis u izlaznu datoteku.
*/
#include <iostream>
#include <fstream>
#include <regex>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <numeric>

using namespace std;

class Student {
private:
    string firstname;
    string lastname;
    string index;
    vector<int> grades;
    double avg_grade;
public:
    Student() {}
    Student(string name, string surname, string idx) {
        firstname = name;
        lastname = surname;
        index = idx;
    }

    void setGrades(vector<int> grds) {
        grades = grds;
        calculateAvgGrade();
    }

    string getFirstname() {
        return firstname;
    }

    string getLastname(){
        return lastname;
    }

    string getIndex() {
        return index;
    }

    double getAvgGrade(){
        return avg_grade;
    }

    friend ostream& operator<<(ostream& os, Student& s) {
      os << s.firstname << " " << s.lastname << " " << s.index;
      os << " ocene: ";
      for(auto it = s.grades.begin(); it != s.grades.end(); it++) {
        os << *it << " ";
      }
      os  << "prosek: " << s.avg_grade << endl;
      return os;
   }
private:
    void calculateAvgGrade(){
        avg_grade = accumulate(grades.begin(), grades.end(), 0.)/grades.size();
    }
};

template<typename T>
class Data {
private:
    mutex data_mtx;
    vector<T> data;
    bool end;
    int data_producers_num;
    condition_variable data_read;
public:
    Data(): end(false), data_producers_num(0) {}
    void add_data(T data_element) {
        unique_lock<mutex> u(data_mtx);
        data.push_back(data_element);
        data_read.notify_one();
    }
    bool remove_data(T &data_element) {
        unique_lock<mutex> u(data_mtx);
        data_read.wait(u, [this]{return !data.empty() || end;});

        if (the_end()) {
            return false;
        }

        data_element = data.back();
        data.pop_back();

        return true;
    }

    bool the_end() {
        return data.empty() && end;
    }

    void add_worker() {
        unique_lock<mutex> u(data_mtx);
        data_producers_num++;
    }

    void remove_worker() {
        unique_lock<mutex> u(data_mtx);
        if (!(--data_producers_num)) {
            end = true;
            data_read.notify_all();
        }
    }

};

void reader(string input_file_name, Data<string>& raw_data) {
    ifstream input_fs(input_file_name);
    raw_data.add_worker();
    while(!input_fs.eof()){
        string s;
        getline(input_fs, s);
        raw_data.add_data(s);
    }
    input_fs.close();
    raw_data.remove_worker();
}

void proccessing_data(Data<string>& raw_data, Data<Student>& proccessed_data){
    regex r("([[:alpha:]]+),([[:alpha:]]+),([[:alpha:]][[:alnum:]]{1,2}\\s[[:digit:]]{1,3}\\/[[:digit:]]{4})");
    regex gradeRegex(",([6-9]|10)");
    smatch match;
    smatch matchGrades;
    vector<int> grades;
    proccessed_data.add_worker();
    string s;
    while (raw_data.remove_data(s)) {
        if(regex_search(s, match, r)) {
            Student student = Student(match[1], match[2], match[3]);
            while(regex_search(s, matchGrades, gradeRegex)) {
                grades.push_back(atoi(matchGrades[1].str().c_str()));
                s = matchGrades.suffix();
            }
            student.setGrades(grades);
            proccessed_data.add_data(student);
            grades.resize(0);
        }
    }
    proccessed_data.remove_worker();
}

void writer(Data<Student>& proccessed_data) {
    ofstream output_fs_stip("kandidati_stipendija.csv");
    ofstream output_fs_kred("kandidati_kredit.csv");
    ofstream output_fs_ostali("ostali.csv");
    Student student;
    while (proccessed_data.remove_data(student)) {
        if(student.getAvgGrade() > 9.0) {
            if(student.getIndex() != "") {
                output_fs_stip << student.getFirstname() << "," << student.getLastname() << "," << student.getIndex() << "," << student.getAvgGrade() << "\n";
            } else {
                output_fs_stip << student.getFirstname() << "," << student.getLastname() << "," << student.getAvgGrade() << "\n";
            }
        } else if(student.getAvgGrade() > 8.0 && student.getAvgGrade() <= 9.0) {
            if(student.getIndex() != "") {
                output_fs_kred << student.getFirstname() << "," << student.getLastname() << "," << student.getIndex() << "," << student.getAvgGrade() << "\n";
            } else {
                output_fs_kred << student.getFirstname() << "," << student.getLastname() << "," << student.getAvgGrade() << "\n";
            }
        } else {
            if(student.getIndex() != "") {
                output_fs_ostali << student.getFirstname() << "," << student.getLastname() << "," << student.getIndex() << "," << student.getAvgGrade() << "\n";
            } else {
                output_fs_ostali << student.getFirstname() << "," << student.getLastname() << "," << student.getAvgGrade() << "\n";
            }
        }
    }
    output_fs_stip.close();
    output_fs_kred.close();
    output_fs_ostali.close();
}


int main() {
    Data<string> raw_data;
    Data<Student> proccessed_data;

    thread th_reader(reader, "studenti.csv", ref(raw_data));
    thread th_writer(writer, ref(proccessed_data));
    thread th_workers[10];

    for(auto &th: th_workers){
        th = thread(proccessing_data, ref(raw_data), ref(proccessed_data));
    }

    th_reader.join();
    for(auto &th: th_workers) {
        th.join();
    }
    th_writer.join();
    
    return 0;
}