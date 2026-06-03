#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio> // для sscanf и snprintf

using namespace std;

// константы для текстовых файлов
const int TEST_FILES_COUNT = 20;
const char* const TEST_FILES[TEST_FILES_COUNT] = {
    "test1.txt",  // самый обычный
    "test2.txt",  // только один аэродром
    "test3.txt",  // граничное время
    "test4.txt",  // один борт с разными марками
    "test5.txt",  // длинные названия марок ЛА
    "test6.txt",  // разные аэродромы, но одинаковое время
    "test7.txt",  // обратный порядок времени
    "test8.txt",  // данные с лишними пробелами
    "test9.txt",  // однозначные часы и минуты
    "test10.txt", // большой набор данных без ошибок
    "test11.txt", // некорректные часы
    "test12.txt", // некорректные минуты
    "test13.txt", // буквы вместо цифр
    "test14.txt", // лишние слова в строке
    "test15.txt", // нехватка параметров в строке
    "test16.txt", // неправильный формат
    "test17.txt", // неправильные аэродромы
    "test18.txt", // 2 одинаковых БН садятся в одну минуту
    "test19.txt", // пустой файл
    "test20.txt"  // пустые строки, символы и тд
};

// cтруктура данных с использованием статических массивов символов
struct Airplane {
    char mark[64];
    char boardNum[32];
    char timeStr[16];
    int timeMin;
    char airfieldStr[32];
    int airfield;
    
    bool isValid; // флаг корректности данных
    char errorMsg[128]; // текст ошибки для вывода перед таблицей
    int lineNum; // исходный номер строки
};

// прототипы функций
void printHeader(); // шапка
int getLength(const char* s); // вычисление длины строки без учета доп байтов
void printCell(const char* text, int width); // форматированный вывод текста в ячейку заданного размера
bool checkBoardNum(const char* b); // проверка БН на формат Б-хххх
bool checkTime(const char* t, int &mins); // проверка времени и перевод в минуты
bool checkAirfield(const char* a, int &num); // проверка аэродрома
void checkSemanticErrors(Airplane* planes, int count); // выявление смысловых дубликатов
void sortIndices(Airplane* planes, int* indices, int count); // индексная сортировка
void printTable(Airplane* planes, int* indices, int count); // вывод списка ошибок и итоговой таблицы

// main
int main() {
    printHeader();

    cout << "Доступные тесты:\n";
    for (int i = 0; i < TEST_FILES_COUNT; i++) {
        cout << i + 1 << ". " << TEST_FILES[i] << "\n";
    }
    cout << "Выберите номер файла (1-" << TEST_FILES_COUNT << "): ";
    int choice;
    cin >> choice;

    if (choice < 1 || choice > TEST_FILES_COUNT) {
        cout << "Некорректный выбор, конец работы.\n";
        return 1;
    }

    const char* filename = TEST_FILES[choice - 1];
    ifstream fin(filename);
    if (!fin.is_open()) {
        cout << "Невозможно открыть файл " << filename << ".\n";
        return 1;
    }

    int max_size = 100;
    // динамическое выделение памяти
    Airplane* planes = new Airplane[max_size];
    int* indices = new int[max_size];
    int count = 0;

    char line[256];
    int lineCounter = 0;

    while (fin.getline(line, 256)) {
        lineCounter++;
        if (strlen(line) == 0 || line[0] == '\n' || line[0] == '\r') { // игнорирование пустых строк
            continue;
        }

        if (count >= max_size) break;

        Airplane current;
        current.lineNum = lineCounter;
        current.isValid = true;
        current.errorMsg[0] = '\0';
        current.timeMin = 0;
        current.airfield = -1;

        char extra[64] = "";
        
        // разбор строки через sscanf
        int fieldsRead = sscanf(line, "%63s %31s %15s %31s %63s", current.mark, current.boardNum, current.timeStr, current.airfieldStr, extra);

        if (fieldsRead != 4) {
            current.isValid = false;
            strcpy(current.errorMsg, "Неверное количество данных (ровно 4 параметра)");
        } else {
            if (!checkBoardNum(current.boardNum)) {
                current.isValid = false;
                strcpy(current.errorMsg, "Некорректный БН (формат Б-хххх)");
            }
            else if (!checkTime(current.timeStr, current.timeMin)) {
                current.isValid = false;
                strcpy(current.errorMsg, "Некорректное время (от 00:00 до 23:59)");
            }
            else if (!checkAirfield(current.airfieldStr, current.airfield)) {
                current.isValid = false;
                strcpy(current.errorMsg, "Некорректный аэродром");
            }
        }

        // сохранение записи при наличии ошибок
        planes[count] = current;
        indices[count] = count;
        count++;
    }
    fin.close();

    checkSemanticErrors(planes, count);
    sortIndices(planes, indices, count);
    printTable(planes, indices, count);

    // очистка памяти
    delete[] planes;
    delete[] indices;

    return 0;
}

// функции

void printHeader() {
    cout << "*******************************************************************************\n";
    cout << "*                      ОЗНАКОМИТЕЛЬНАЯ ПРАКТИКА                               *\n";
    cout << "*-----------------------------------------------------------------------------*\n";
    cout << "* Project Type  : Standard C++ Console App                                    *\n";
    cout << "* File Name     : ozn_pr.cpp                                                  *\n";
    cout << "* Programmer(s) : Бефорд М. А.                                                *\n";
    cout << "* Modifyed By   : -                                                           *\n";
    cout << "* Created       : 28/04/26                                                    *\n";
    cout << "* Last Revision : 26/05/26                                                    *\n";
    cout << "* Comment(s)    : -                                                           *\n";
    cout << "*******************************************************************************\n\n";
}

int getLength(const char* s) {
    int len = 0;
    int i = 0;
    while (s[i] != '\0') {
        if ((s[i] & 0xC0) != 0x80) len++; // убрали доп байты кириллицы
        i++;
    }
    return len;
}

void printCell(const char* text, int width) {
    cout << text;
    int spaces = width - getLength(text); // считаем сколько пробелов не хватает
    for (int i = 0; i < spaces; i++) cout << " "; // добиваем ячейку пробелами
}

bool checkBoardNum(const char* b) {
    const char* prefix = "Б-";
    int prefLen = (int)strlen(prefix);
    
    // проверка наличия префикса Б-
    if (strncmp(b, prefix, prefLen) != 0) return false;
    
    // после префикса должно идти ровно 4 символа
    int totalLen = (int)strlen(b);
    if (totalLen != prefLen + 4) return false;
    
    // проверка, что эти 4 символа это цифры
    for (int i = prefLen; i < totalLen; i++) {
        if (b[i] < '0' || b[i] > '9') return false;
    }
    return true;
}

bool checkTime(const char* t, int &mins) {
    if (strlen(t) != 5 || t[2] != ':') return false; // проверка длины и двоеточия
    
    for (int i = 0; i < 5; i++) {
        if (i == 2) continue;
        if (t[i] < '0' || t[i] > '9') return false; // проверка на цифры
    }
    
    // перевод символов в числа
    int h = (t[0] - '0') * 10 + (t[1] - '0');
    int m = (t[3] - '0') * 10 + (t[4] - '0');
    
    if (h < 0 || h > 23 || m < 0 || m > 59) return false; // проверка времени
    
    mins = h * 60 + m; // перевод в минуты для сортировки
    return true;
}

bool checkAirfield(const char* a, int &num) {
    const char* prefix = "АП";
    int prefLen = (int)strlen(prefix);
    
    if (strncmp(a, prefix, prefLen) != 0) return false; // проверка на наличие префикса АП
    if ((int)strlen(a) != prefLen + 1) return false; // после АП должна быть ровно 1 цифра
    
    char digit = a[prefLen];
    if (digit >= '1' && digit <= '3') { // разрешены только 1, 2, 3
        num = digit - '0';
        return true;
    }
    return false;
}

void checkSemanticErrors(Airplane* planes, int count) {
    for (int i = 0; i < count; i++) {
        if (!planes[i].isValid) continue;
        for (int j = i + 1; j < count; j++) {
            if (!planes[j].isValid) continue;

            if (strcmp(planes[i].boardNum, planes[j].boardNum) == 0) {
                // два самолета с одинаковым бн в одно и то же время
                if (planes[i].timeMin == planes[j].timeMin) {
                    planes[i].isValid = false;
                    planes[j].isValid = false;
                    strcpy(planes[i].errorMsg, "Дублирование времени посадки для одного БН");
                    strcpy(planes[j].errorMsg, "Дублирование времени посадки для одного БН");
                }
            }
        }
    }
}

void sortIndices(Airplane* planes, int* indices, int count) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            int id1 = indices[j];
            int id2 = indices[j + 1];

            bool swapNeeded = false;

            // невалидные записи всплывают в начало списка
            if (planes[id1].isValid && !planes[id2].isValid) {
                swapNeeded = true;
            } 
            // если обе норм, сорт по аэродромам
            else if (planes[id1].isValid && planes[id2].isValid) {
                if (planes[id1].airfield > planes[id2].airfield) {
                    swapNeeded = true;
                } 
                // если аэродромы одинаковые, сорт по времени
                else if (planes[id1].airfield == planes[id2].airfield) {
                    if (planes[id1].timeMin > planes[id2].timeMin) {
                        swapNeeded = true;
                    }
                }
            }

            // меняем местами индексы
            if (swapNeeded) {
                int temp = indices[j];
                indices[j] = indices[j + 1];
                indices[j + 1] = temp;
            }
        }
    }
}

void printTable(Airplane* planes, int* indices, int count) {
    if (count == 0) {
        cout << "Данных для вывода нет.\n";
        return;
    }

    // вывод ошибок перед таблицей
    bool hasErrors = false;
    for (int i = 0; i < count; i++) {
        if (!planes[i].isValid) {
            if (!hasErrors) {
                cout << "--- СПИСОК ОШИБОК В ИСХОДНЫХ ДАННЫХ ---\n";
                hasErrors = true;
            }
            cout << "Строка " << planes[i].lineNum << ": " << planes[i].errorMsg << "\n";
        }
    }
    if (hasErrors) cout << "\n";

    int currentAirfield = -1;
    int orderNum = 1;
    bool badHeaderPrinted = false;

    for (int i = 0; i < count; i++) {
        int idx = indices[i];

        // блок некорректных записей
        if (!planes[idx].isValid) {
            if (!badHeaderPrinted) {
                cout << "=================================================================\n";
                cout << "         ОТКЛОНЕННЫЕ ЗАПИСИ (НЕ УЧАСТВУЮТ В СОРТИРОВКЕ)\n";
                cout << "=================================================================\n";
                printCell("№", 6);
                printCell("Марка ЛА", 20);
                printCell("Бортовой номер", 20);
                cout << "Время   Аэродром\n";
                cout << "-----------------------------------------------------------------\n";
                badHeaderPrinted = true;
            }
            char numStr[12];
            snprintf(numStr, sizeof(numStr), "%d", orderNum++); // безопасный перевод числа в строку
            printCell(numStr, 6);
            printCell(planes[idx].mark, 20);
            printCell(planes[idx].boardNum, 20);
            cout << planes[idx].timeStr << "   " << planes[idx].airfieldStr << "\n";
        } 

        // блок корректных записей
        else {
            if (planes[idx].airfield != currentAirfield) {
                currentAirfield = planes[idx].airfield;
                orderNum = 1; // обнул счетчика для нового аэродрома
                
                cout << "\n=================================================================\n";
                cout << "                       АЭРОДРОМ № " << currentAirfield << "\n";
                cout << "=================================================================\n";
                printCell("№", 6);
                printCell("Марка ЛА", 20);
                printCell("Бортовой номер", 20);
                cout << "Время\n";
                cout << "-----------------------------------------------------------------\n";
            }
            char numStr[12];
            snprintf(numStr, sizeof(numStr), "%d", orderNum++); // безопасный перевод числа в строку
            printCell(numStr, 6);
            printCell(planes[idx].mark, 20);
            printCell(planes[idx].boardNum, 20);
            cout << planes[idx].timeStr << "\n";
        }
    }
    cout << "=================================================================\n";
}
