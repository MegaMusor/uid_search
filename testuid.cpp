#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <locale>  

using namespace std;

// Класс для представления записи с UID (7 байт)
class Record {
private:
    string uid;  // 7 байт
    string data; // произвольные данные
    
public:
    Record(const string& uid, const string& data) 
        : uid(uid), data(data) {
        if (uid.length() != 7) {
            throw invalid_argument("UID должен быть длиной ровно 7 байт");
        }
    }
    
    const string& getUid() const { return uid; }
    const string& getData() const { return data; }
};

// Класс для управления базой данных с эффективным поиском
class Database {
private:
    unordered_map<string, Record*> index;
    vector<Record> records;
    
public:
    ~Database() {
        // Очищаем указатели в index, так как записи хранятся в vector
        index.clear();
    }
    
    // Добавление записи в базу данных
    void addRecord(Record&& record) {
        records.push_back(move(record));
        // Сохраняем указатель на только что добавленную запись
        index[records.back().getUid()] = &records.back();
    }
    
    // Поиск записи по UID
    Record* findRecord(const string& uid) {
        auto it = index.find(uid);
        if (it != index.end()) {
            return it->second;
        }
        return nullptr; 
    }
    
    
    size_t size() const {
        return records.size();
    }
    
    
    void clear() {
        records.clear();
        index.clear();
    }
};

// Генератор случайных UID (7 байт)
class UidGenerator {
private:
    random_device rd;
    mt19937 gen;
    uniform_int_distribution<int> dist;
    
public:
    UidGenerator() : gen(rd()), dist(0, 255) {}
    
    string generateUid() {
        string uid;
        uid.reserve(7);
        
        for (int i = 0; i < 7; ++i) {
            uid += static_cast<char>(dist(gen));
        }
        
        return uid;
    }
};


string formatNumber(size_t number) {
    string str = to_string(number);
    int n = str.length() - 3;
    while (n > 0) {
        str.insert(n, " ");
        n -= 3;
    }
    return str;
}


void runPerformanceTest() {
    const int TOTAL_RECORDS = 100000;
    const int SEARCH_TESTS = 10000;
    
    Database db;
    UidGenerator uidGen;
    
    cout << "=== ТЕСТИРОВАНИЕ БАЗЫ ДАННЫХ ===" << endl;
    cout << "Генерация " << formatNumber(TOTAL_RECORDS) << " записей..." << endl;
    
    // Генерация уникальных UID
    unordered_map<string, bool> usedUids;
    auto startTime = chrono::high_resolution_clock::now();
    
    for (int i = 0; i < TOTAL_RECORDS; ++i) {
        string uid;
       
        do {
            uid = uidGen.generateUid();
        } while (usedUids.count(uid) > 0);
        
        usedUids[uid] = true;
        string data = "Данные для записи " + to_string(i + 1);
        db.addRecord(Record(uid, data));
        
        
        if ((i + 1) % 10000 == 0) {
            cout << "Сгенерировано записей: " << formatNumber(i + 1) << endl;
        }
    }
    
    auto endTime = chrono::high_resolution_clock::now();
    auto generationTime = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
    cout << "Генерация завершена за " << generationTime.count() << " мс" << endl;
    
    
    cout << "\nПодготовка тестовых ключей для поиска..." << endl;
    vector<string> searchKeys;
    vector<string> existingUids;
    
    
    for (const auto& pair : usedUids) {
        existingUids.push_back(pair.first);
    }
    
    // 70% существующих ключей, 30% случайных (несуществующих)
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> existDist(0, existingUids.size() - 1);
    
    for (int i = 0; i < SEARCH_TESTS; ++i) {
        if (i < SEARCH_TESTS * 0.7) {
           
            searchKeys.push_back(existingUids[existDist(gen)]);
        } else {
            
            searchKeys.push_back(uidGen.generateUid());
        }
    }
    
    
    shuffle(searchKeys.begin(), searchKeys.end(), gen);
    
    
    cout << "Тестирование поиска " << formatNumber(SEARCH_TESTS) << " ключей..." << endl;
    
    int foundCount = 0;
    int notFoundCount = 0;
    
    startTime = chrono::high_resolution_clock::now();
    
    for (int i = 0; i < SEARCH_TESTS; ++i) {
        Record* record = db.findRecord(searchKeys[i]);
        if (record) {
            foundCount++;
        } else {
            notFoundCount++;
        }
        
        
        if (SEARCH_TESTS > 1000 && (i + 1) % 1000 == 0) {
            cout << "Выполнено поисков: " << formatNumber(i + 1) << endl;
        }
    }
    
    endTime = chrono::high_resolution_clock::now();
    auto searchTime = chrono::duration_cast<chrono::microseconds>(endTime - startTime);
    
    // Статистика
    cout << "\n=== РЕЗУЛЬТАТЫ ТЕСТИРОВАНИЯ ===" << endl;
    cout << "Общая статистика:" << endl;
    cout << "  Всего записей в базе: " << formatNumber(db.size()) << endl;
    cout << "  Выполнено тестов поиска: " << formatNumber(SEARCH_TESTS) << endl;
    cout << "  Найдено записей: " << formatNumber(foundCount) << endl;
    cout << "  Не найдено записей: " << formatNumber(notFoundCount) << endl;
    
    cout << "\nПроизводительность поиска:" << endl;
    cout << "  Общее время поиска: " << searchTime.count() << " мкс" << endl;
    cout << "  Среднее время на поиск: " 
              << fixed << setprecision(3) 
              << (static_cast<double>(searchTime.count()) / SEARCH_TESTS) 
              << " мкс" << endl;
    cout << "  Поисков в секунду: " 
              << formatNumber(static_cast<long long>((SEARCH_TESTS * 1000000.0) / searchTime.count()))
              << endl;
    
    cout << "\nЭффективность:" << endl;
    double speed = (SEARCH_TESTS * 1000000.0) / searchTime.count();
    cout << "  Скорость обработки: " << fixed << setprecision(0) << speed << " операций/сек" << endl;
    
    // Сравнение с линейным поиском
    double linearSearchTime = (TOTAL_RECORDS / 2.0) * SEARCH_TESTS * 0.0001; // примерная оценка
    double speedup = linearSearchTime / (searchTime.count() / 1000000.0);
    cout << "  Ускорение относительно линейного поиска: ~" << formatNumber(static_cast<size_t>(speedup)) << " раз" << endl;
}


void demonstration() {
    cout << "\n=== ДЕМОНСТРАЦИОННЫЙ ПРИМЕР ===" << endl;
    
    Database db;
    
  
    db.addRecord(Record("ABCDEFG", "Тестовая запись 1"));
    db.addRecord(Record("HIJKLMN", "Тестовая запись 2"));
    db.addRecord(Record("OPQRSTU", "Тестовая запись 3"));
    
    
    Record* found = db.findRecord("ABCDEFG");
    if (found) {
        cout << "Найдена запись: UID=" << found->getUid() 
                  << ", Данные=" << found->getData() << endl;
    }
    
   
    Record* notFound = db.findRecord("XXXXXXX");
    if (!notFound) {
        cout << "Запись с UID=XXXXXXX не найдена (ожидаемо)" << endl;
    }
    
    cout << "Всего записей в демо-базе: " << db.size() << endl;
}

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    
    cout << "=== СИСТЕМА ПОИСКА В БАЗЕ ДАННЫХ ПО UID ===" << endl;
    cout << "Реализация с использованием хэш-таблицы для эффективного поиска" << endl;
    
    try {
        
        demonstration();
        
        
        runPerformanceTest();
        
    } catch (const exception& e) {
        cerr << "Ошибка выполнения: " << e.what() << endl;
        return 1;
    }
    
    cout << "\n=== ТЕСТИРОВАНИЕ ЗАВЕРШЕНО ===" << endl;
    return 0;
}