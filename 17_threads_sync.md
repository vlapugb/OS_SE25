# Синхронизация нитей: mutex, condvars, once, barrier и др.

## 1. Mutex (мьютекс)

### Определение

`std::mutex` --- механизм исключительного доступа, позволяющий защитить
общие ресурсы от одновременного использования несколькими потоками.

### Основные операции

-   `lock()` --- захват мьютекса\
-   `unlock()` --- освобождение\
-   `try_lock()` --- попытка захвата без блокировки\
-   вариации: `std::timed_mutex`, `std::recursive_mutex`,
    `std::shared_mutex`.

### Пример (C++)

``` cpp
#include <iostream>
#include <thread>
#include <mutex>

std::mutex m;
int counter = 0;

void work() {
    for (int i = 0; i < 100000; i++) {
        std::lock_guard<std::mutex> lock(m);
        counter++;
    }
}

int main() {
    std::thread t1(work);
    std::thread t2(work);

    t1.join();
    t2.join();

    std::cout << counter << std::endl;
}
```

### Практика использования

-   Всегда отпирайте мьютекс через RAII (`std::lock_guard` или
    `std::unique_lock`).
-   При захвате нескольких мьютексов используйте одинаковый порядок или
    `std::scoped_lock/std::lock`, чтобы не получить взаимную блокировку.
-   Для коротких критических секций без сна подойдёт спинлок
    (`std::atomic_flag`, `pthread_spinlock_t`), но он грузит CPU.

---

## 2. Условные переменные (Condition Variables)

### Определение

`std::condition_variable` позволяет одному потоку ждать наступления
события, которое генерирует другой поток. Ожидание автоматически
временное выпускает связанный мьютекс и перехватывает его перед выходом
из `wait`.

### Пример

``` cpp
#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>

std::mutex m;
std::condition_variable cv;
bool ready = false;

void worker() {
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, []{ return ready; });
    std::cout << "Work done!" << std::endl;
}

int main() {
    std::thread t(worker);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    {
        std::lock_guard<std::mutex> lock(m);
        ready = true;
    }
    cv.notify_one();

    t.join();
}
```

### Важные детали

-   Используйте `wait(lock, predicate)` или цикл `while(!ready) wait`,
    чтобы защититься от ложных пробуждений и гонок между проверкой и
    сигналом.
-   `notify_one` будит один ожидающий поток, `notify_all` — всех.
-   `condition_variable` работает только с `std::unique_lock`; для
    атомарных флагов без мьютекса подойдёт `condition_variable_any` или
    `std::atomic` + busy-wait.

---

## 3. std::once_flag и std::call_once

### Назначение

Используется для выполнения инициализации один раз независимо от
количества потоков.

### Пример

``` cpp
#include <iostream>
#include <thread>
#include <mutex>

std::once_flag flag;

void init() {
    std::cout << "Initialized once!" << std::endl;
}

void worker() {
    std::call_once(flag, init);
}

int main() {
    std::thread t1(worker);
    std::thread t2(worker);
    std::thread t3(worker);

    t1.join();
    t2.join();
    t3.join();
}
```

**Важно:** если функция, переданная в `call_once`, выбрасывает исключение
или завершится с ошибкой, следующая попытка снова выполнит её, пока одна
не завершится успешно.

---

## 4. Barrier (барьер)

### Определение

Барьер синхронизирует несколько потоков: каждый должен дойти до
синхронизирующей точки, прежде чем продолжать. В C++20 барьер
многоразовый и может выполнять «шлюзовую» функцию после прихода всех
участников.

### Пример (C++20)

``` cpp
#include <barrier>
#include <iostream>
#include <thread>

std::barrier sync_point(3);

void worker(int id) {
    std::cout << "Thread " << id << " reached barrier" << std::endl;
    sync_point.arrive_and_wait();
    std::cout << "Thread " << id << " passed barrier" << std::endl;
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    std::thread t3(worker, 3);

    t1.join();
    t2.join();
    t3.join();
}
```

**Замечания:**

-   Если один поток не дойдёт до барьера, остальные зависнут —
    используйте тайм-ауты или контроль ошибок, когда это возможно.
-   `std::latch` — одноразовый вариант, подходит для ожидания старта или
    завершения группы задач.

---

# Задача о философах (Dining Philosophers)

## Классическая постановка

Пять философов сидят за круглым столом. Между каждым находится вилка.
Чтобы поесть, философу нужны две вилки.

### Проблемы:

-   взаимная блокировка (deadlock)\
-   голодание (starvation)

## Пример решения на C++

Используем мьютексы и строгий порядок взятия вилок.

``` cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <array>

constexpr int N = 5;
std::array<std::mutex, N> forks;

void philosopher(int id) {
    int left = id;
    int right = (id + 1) % N;

    if (left > right) std::swap(left, right); // предотвращает deadlock

    while (true) {
        {
            std::lock(forks[left], forks[right]);
            std::lock_guard<std::mutex> lock1(forks[left], std::adopt_lock);
            std::lock_guard<std::mutex> lock2(forks[right], std::adopt_lock);

            std::cout << "Philosopher " << id << " is eating" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << "Philosopher " << id << " is thinking" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    std::array<std::thread, N> threads;
    for (int i = 0; i < N; i++)
        threads[i] = std::thread(philosopher, i);

    for (auto& t : threads)
        t.join();
}
```

---

# Работа с общими ресурсами в многопоточной программе

-   Потоки процесса разделяют память и большинство ресурсов, поэтому
    **всегда** синхронизируйте доступ к изменяемым структурам.
-   Ввод-вывод из нескольких потоков может перемешиваться; для файловых
    дескрипторов используйте мьютекс или флаги `O_APPEND`/`std::ios` с
    буферизацией, если порядок записей не важен.
-   Проверяйте, что код выхода из критической секции не бросает
    исключений до освобождения блокировки; RAII-обёртки делают это
    проще.
