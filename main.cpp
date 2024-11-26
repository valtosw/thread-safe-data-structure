#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <random>
#include <string>
#include <vector>
#include <shared_mutex>
#include <mutex>
using namespace std;

class DataStruct {
    int fields[2]{};
    shared_mutex mutexes[2];
    shared_mutex string_mutex;
public:
    DataStruct() {
        fields[0] = 0;
        fields[1] = 0;
    }

    int read(const int& index) {
        shared_lock<shared_mutex> lock(mutexes[index]);
        return fields[index];
    }

    void write(const int& index, const int& value) {
        unique_lock<shared_mutex> lock(mutexes[index]);
        fields[index] = value;
    }

    operator string() {
        shared_lock<shared_mutex> lock(string_mutex);

        string result = "(";
        for(int i = 0; i < size(fields); i++) {
            shared_lock<shared_mutex> f_lock(mutexes[i]);
            result += fields[i] + ", ";
        }
        result += ")";

        return result;
    }
};

void generateFile(const string& filename, const vector<double>& frequency, const int& commandsNum) {
    ofstream file(filename);
    random_device rd;
    mt19937 gen(rd());
    discrete_distribution<> dist(frequency.begin(), frequency.end());

    for (int i = 0; i < commandsNum; i++) {
        switch (dist(gen)) {
            case 0: file << "read 0\n"; break;
            case 1: file << "write 0 1\n"; break;
            case 2: file << "read 1\n"; break;
            case 3: file << "write 1 1\n"; break;
            case 4: file << "string\n"; break;
            default: throw invalid_argument("invalid command");
        }
    }

    file.close();
}

void executeCommands(DataStruct& data_struct, const string& filename) {
    ifstream file(filename);
    string command;
    int index, value;

    while (file >> command) {
        if (command == "read") {
            file >> index;
            int num = data_struct.read(index);
        }
        else if (command == "write") {
            file >> index >> value;
            data_struct.write(index, value);
        }
        else if (command == "string") {
            string s = string(data_struct);
        }
    }

    file.close();
}

void measureTime(DataStruct& data_struct, const vector<string>& filenames, const int& threadsNum) {
    vector<thread> threads;

    const auto start = chrono::high_resolution_clock::now();

    for(int i = 0; i < threadsNum; i++) {
        threads.emplace_back(executeCommands, ref(data_struct), ref(filenames[i]));
    }

    for(auto& thread : threads) {
        thread.join();
    }

    const auto end = chrono::high_resolution_clock::now();

    cout << "Number of threads: " << threadsNum << endl;
    cout << "With ";
    for (const string& filename : filenames) {
        cout << filename << " ";
    }

    cout << "\nExecution time: " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms\n" << endl;
}

int main() {
    try {
        DataStruct data_struct;

        const vector<double> frequency_from_task = {0.25, 0.05, 0.05, 0.25, 0.4};
        const vector<double> frequency_equal = {0.2, 0.2, 0.2, 0.2, 0.2};
        const vector<double> frequency_custom = {0.1, 0.3, 0.15, 0.4, 0.05};

        generateFile("from_task1.txt", frequency_from_task, 10e5);
        generateFile("from_task2.txt", frequency_from_task, 10e5);
        generateFile("from_task3.txt", frequency_from_task, 10e5);

        generateFile("equal1.txt", frequency_equal, 10e5);
        generateFile("equal2.txt", frequency_equal, 10e5);
        generateFile("equal3.txt", frequency_equal, 10e5);

        generateFile("custom1.txt", frequency_custom, 10e5);
        generateFile("custom2.txt", frequency_custom, 10e5);
        generateFile("custom3.txt", frequency_custom, 10e5);

        measureTime(data_struct, {"from_task1.txt"}, 1);
        measureTime(data_struct, {"equal1.txt"}, 1);
        measureTime(data_struct, {"custom1.txt"}, 1);

        measureTime(data_struct, {"from_task1.txt", "from_task2.txt"}, 2);
        measureTime(data_struct, {"equal1.txt", "equal2.txt"}, 2);
        measureTime(data_struct, {"custom1.txt", "custom2.txt"}, 2);

        measureTime(data_struct, {"from_task1.txt", "from_task2.txt", "from_task3.txt"}, 3);
        measureTime(data_struct, {"equal1.txt", "equal2.txt", "equal3.txt"}, 3);
        measureTime(data_struct, {"custom1.txt", "custom2.txt", "custom3"}, 3);
    }
    catch(const exception& e) {
        cerr << e.what() << endl;
    }

    return 0;
}
